function butteraugli (cfg) {
	return new Promise(function (resolve, reject) {
		var worker = new Worker("../dominikhlbg/butteraugli.worker.js");
		var heatmapContainer;
		var c = document.createElement('canvas');
		var cx = c.getContext('2d');

		c.width = Math.max(cfg.image1.width, cfg.image2.width);
		c.height = Math.max(cfg.image1.height, cfg.image2.height);

		heatmapContainer = cx.getImageData(0, 0, c.width, c.height);

		worker.postMessage({
			a: cfg.image1,
			b: cfg.image2,
			heatmap: heatmapContainer
		});

		worker.onmessage = function (e) {
			worker.terminate();

			resolve(e.data);
		};
	})
};
