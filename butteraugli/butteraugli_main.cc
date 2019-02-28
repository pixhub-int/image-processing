#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "butteraugli.h"

#include <emscripten.h>

namespace butteraugli {
namespace {

// "rgb": cleared and filled with same-sized image planes (one per channel);
// either RGB, or RGBA if the PNG contains an alpha channel.
bool ReadRGBA(uint8_t* rgba, std::vector<Image8>* rgb, int xsize, int ysize) {

  *rgb = CreatePlanes<uint8_t>(xsize, ysize, 3);
  rgb->push_back(Image8(xsize, ysize));

  for (int y = 0; y < ysize; ++y) {
    uint8_t* const BUTTERAUGLI_RESTRICT row0 = (*rgb)[0].Row(y);
    uint8_t* const BUTTERAUGLI_RESTRICT row1 = (*rgb)[1].Row(y);
    uint8_t* const BUTTERAUGLI_RESTRICT row2 = (*rgb)[2].Row(y);
    uint8_t* const BUTTERAUGLI_RESTRICT row3 = (*rgb)[3].Row(y);

    int offset = xsize * 4 * y;

    for (int x = 0; x < xsize; ++x) {
      row0[x] = rgba[offset + 4 * x + 0];
      row1[x] = rgba[offset + 4 * x + 1];
      row2[x] = rgba[offset + 4 * x + 2];
      row3[x] = rgba[offset + 4 * x + 3];
    }
  }

  return true;
}

const double* NewSrgbToLinearTable() {
  double* table = new double[256];
  for (int i = 0; i < 256; ++i) {
    const double srgb = i / 255.0;
    table[i] =
        255.0 * (srgb <= 0.04045 ? srgb / 12.92
                                 : std::pow((srgb + 0.055) / 1.055, 2.4));
  }
  return table;
}

// Translate R, G, B channels from sRGB to linear space. If an alpha channel
// is present, overlay the image over a black or white background. Overlaying
// is done in the sRGB space; while technically incorrect, this is aligned with
// many other software (web browsers, WebP near lossless).
void FromSrgbToLinear(const std::vector<Image8>& rgb,
                      std::vector<ImageF>& linear, int background) {
  const size_t xsize = rgb[0].xsize();
  const size_t ysize = rgb[0].ysize();
  static const double* const kSrgbToLinearTable = NewSrgbToLinearTable();

    for (int c = 0; c < 3; c++) {
      linear.push_back(ImageF(xsize, ysize));
      for (int y = 0; y < ysize; ++y) {
        const uint8_t* const BUTTERAUGLI_RESTRICT row_rgb = rgb[c].Row(y);
        float* const BUTTERAUGLI_RESTRICT row_linear = linear[c].Row(y);
        const uint8_t* const BUTTERAUGLI_RESTRICT row_alpha = rgb[3].Row(y);
        for (size_t x = 0; x < xsize; x++) {
          int value;
          if (row_alpha[x] == 255) {
            value = row_rgb[x];
          } else if (row_alpha[x] == 0) {
            value = background;
          } else {
            const int fg_weight = row_alpha[x];
            const int bg_weight = 255 - fg_weight;
            value =
                (row_rgb[x] * fg_weight + background * bg_weight + 127) / 255;
          }
          row_linear[x] = kSrgbToLinearTable[value];
        }
      }
    }
}

static void ScoreToRgb(double score, double good_threshold,
                        double bad_threshold, uint8_t rgb[3]) {
  double heatmap[12][3] = {
    { 0, 0, 0 },
    { 0, 0, 1 },
    { 0, 1, 1 },
    { 0, 1, 0 }, // Good level
    { 1, 1, 0 },
    { 1, 0, 0 }, // Bad level
    { 1, 0, 1 },
    { 0.5, 0.5, 1.0 },
    { 1.0, 0.5, 0.5 },  // Pastel colors for the very bad quality range.
    { 1.0, 1.0, 0.5 },
    { 1, 1, 1, },
    { 1, 1, 1, },
  };
  if (score < good_threshold) {
    score = (score / good_threshold) * 0.3;
  } else if (score < bad_threshold) {
    score = 0.3 + (score - good_threshold) /
        (bad_threshold - good_threshold) * 0.15;
  } else {
    score = 0.45 + (score - bad_threshold) /
        (bad_threshold * 12) * 0.5;
  }
  static const int kTableSize = sizeof(heatmap) / sizeof(heatmap[0]);
  score = std::min<double>(std::max<double>(
      score * (kTableSize - 1), 0.0), kTableSize - 2);
  int ix = static_cast<int>(score);
  double mix = score - ix;
  for (int i = 0; i < 3; ++i) {
    double v = mix * heatmap[ix + 1][i] + (1 - mix) * heatmap[ix][i];
    rgb[i] = static_cast<uint8_t>(255 * pow(v, 0.5) + 0.5);
  }
}

void CreateHeatMapImage(const ImageF& distmap, double good_threshold,
                        double bad_threshold, size_t xsize, size_t ysize,
                        std::vector<uint8_t>* heatmap) {
  heatmap->resize(3 * xsize * ysize);
  for (size_t y = 0; y < ysize; ++y) {
    for (size_t x = 0; x < xsize; ++x) {
      int px = xsize * y + x;
      double d = distmap.Row(y)[x];
      uint8_t* rgb = &(*heatmap)[3 * px];
      ScoreToRgb(d, good_threshold, bad_threshold, rgb);
    }
  }
}

// main() function, within butteraugli namespace for convenience.
double Run(uint8_t* rgba1, uint8_t* rgba2, int width, int height, uint8_t* heatmap) {
  bool rgba1HasTransparency = false;
  bool rgba2HasTransparency = false;

  for (int i = 0; i < sizeof(rgba1); i += 4) {
    if (rgba1[i + 3] < 255) {
      rgba1HasTransparency = true;
      break;
    }
  }

  for (int i = 0; i < sizeof(rgba2); i += 4) {
    if (rgba2[i + 3] < 255) {
      rgba2HasTransparency = true;
      break;
    }
  }

  std::vector<Image8> rgb1;
  std::vector<Image8> rgb2;

  ReadRGBA(rgba1, &rgb1, width, height);
  ReadRGBA(rgba2, &rgb2, width, height);

  for (size_t c = 0; c < rgb1.size(); ++c) {
    if (rgb1[c].xsize() != rgb2[c].xsize() ||
        rgb1[c].ysize() != rgb2[c].ysize()) {
      fprintf(
          stderr, "The images are not equal in size: (%lu,%lu) vs (%lu,%lu)\n",
          rgb1[c].xsize(), rgb2[c].xsize(), rgb1[c].ysize(), rgb2[c].ysize());
      return 1.0;
    }
  }

  // TODO: Figure out if it is a good idea to fetch the gamma from the image
  // instead of applying sRGB conversion.
  std::vector<ImageF> linear1, linear2;

  // Overlay the image over a black background.
  FromSrgbToLinear(rgb1, linear1, 0);
  FromSrgbToLinear(rgb2, linear2, 0);

  ImageF diff_map, diff_map_on_white;
  double diff_value;

  if (!butteraugli::ButteraugliInterface(linear1, linear2, 1.0,
                                         diff_map, diff_value)) {
    fprintf(stderr, "Butteraugli comparison failed\n");
    return 1.0;
  }

  ImageF* diff_map_ptr = &diff_map;

  if (rgba1HasTransparency || rgba2HasTransparency) {
    // If the alpha channel is present, overlay the image over a white
    // background as well.
    FromSrgbToLinear(rgb1, linear1, 255);
    FromSrgbToLinear(rgb2, linear2, 255);
    double diff_value_on_white;
    if (!butteraugli::ButteraugliInterface(linear1, linear2, 1.0,
                                           diff_map_on_white,
                                           diff_value_on_white)) {
      fprintf(stderr, "Butteraugli comparison failed\n");
      return 1.0;
    }
    if (diff_value_on_white > diff_value) {
      diff_value = diff_value_on_white;
      diff_map_ptr = &diff_map_on_white;
    }
  }
  
  free(rgba1);
  free(rgba2);

    const double good_quality = ::butteraugli::ButteraugliFuzzyInverse(1.5);
    const double bad_quality = ::butteraugli::ButteraugliFuzzyInverse(0.5);
    std::vector<uint8_t> rgb;
    CreateHeatMapImage(*diff_map_ptr, good_quality, bad_quality,
                       rgb1[0].xsize(), rgb2[0].ysize(), &rgb);

    int j = 0;

    for (int i = 0; i < rgb.size(); i += 3) {
      heatmap[j + 0] = rgb[i + 0];
      heatmap[j + 1] = rgb[i + 1];
      heatmap[j + 2] = rgb[i + 2];
      heatmap[j + 3] = 255;
      j += 4;
    }

    return diff_value;
}

}  // namespace
}  // namespace butteraugli
int main(int argc, char** argv) {}

extern "C" {
double butteraugli_output(uint8_t* rgba1, uint8_t* rgba2, int width, int height, uint8_t* heatmap) {
return butteraugli::Run(
    rgba1,
    rgba2,
    width,
    height,
    heatmap
  );
}

void mfree(uint8_t* p) {
  free(p);
}
}
