function butteraugli (cfg) {
	if (butteraugli.__worker) {
		butteraugli.__worker.terminate();
	};
	butteraugli.__worker = null;

	return new Promise(function (resolve, reject) {
		butteraugli.__worker = new Worker("../butteraugli/butteraugli-wrapper.worker.js");

		var heatmapContainer;
		var c = document.createElement('canvas');
		var cx = c.getContext('2d');

		c.width = Math.max(cfg.image1.width, cfg.image2.width);
		c.height = Math.max(cfg.image1.height, cfg.image2.height);

		heatmapContainer = cx.getImageData(0, 0, c.width, c.height);

		butteraugli.__worker.postMessage({
			a: cfg.image1,
			b: cfg.image2,
			heatmap: heatmapContainer
		});

		butteraugli.__worker.onmessage = function (e) {
			if (butteraugli.__worker) {
				butteraugli.__worker.terminate();
			};
			butteraugli.__worker = null;

			resolve(e.data);
		};
	})
};
