importScripts('butteraugli_emscripten.js');

onmessage = function (e) {
	Module.onRuntimeInitialized = function () {
		var image1 = e.data.a;
		var image2 = e.data.b;
		var heatmapContainer = e.data.heatmap;
		var heatmapSize = image1.width * image1.height * 3;
	
		var heatmap = Module._malloc(heatmapSize);
		var image1arr = Module._malloc(image1.data.length);
	
		Module.HEAPU8.set(image1.data, image1arr);
	
		var image2arr = Module._malloc(image2.data.length);
	
		Module.HEAPU8.set(image2.data, image2arr);

		var start = performance.now();
		var score = Module._butteraugli_output(
			image1arr,
			image1.width,
			image1.height,
			image2arr,
			image2.width,
			image2.height,
			heatmap
		);
		var finish = performance.now() - start;
	
		var heatmapdata3bytes = new Uint8Array(heatmapSize);
	
		heatmapdata3bytes.set(Module.HEAPU8.subarray(heatmap,heatmap+heatmapSize));
		
		for(var i=0,a=0;i<heatmapdata3bytes.length;a+=4,i+=3) {
			heatmapContainer.data[a+0]=heatmapdata3bytes[i+0];
			heatmapContainer.data[a+1]=heatmapdata3bytes[i+1];
			heatmapContainer.data[a+2]=heatmapdata3bytes[i+2];
			heatmapContainer.data[a+3]=255;
		};
	
		postMessage({
			distance: score,
			heatmap: heatmapContainer,
			duration: finish
		});
	};
};
