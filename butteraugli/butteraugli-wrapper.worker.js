importScripts('butteraugli.js');

onmessage = function (e) {
	Module.onRuntimeInitialized = function () {
		var api = {
			butteraugli_output:  Module.cwrap('butteraugli_output', 'number', ['number', 'number', 'number', 'number', 'number']),
			mfree: Module.cwrap('mfree', '', ['number']),
		};

		var i1 = e.data.a;
		var i2 = e.data.b;

		var heatmapContainer = e.data.heatmap;
		var size = i1.width * i1.height * 4;
		var heatPointer = Module._malloc(size);

		var i1arr = Module._malloc(size);
		Module.HEAPU8.set(i1.data, i1arr);
	
		var i2arr = Module._malloc(size);
		Module.HEAPU8.set(i2.data, i2arr);

		var start = performance.now();
		var score = Module._butteraugli_output(
			i1arr,
			i2arr,
			i1.width,
			i1.height,
			heatPointer
		);
		var finish = performance.now() - start;
	
		heatmapContainer.data.set(Module.HEAPU8.subarray(heatPointer,heatPointer + size));

		api.mfree(heatPointer);
	
		postMessage({
			distance: score,
			heatmap: heatmapContainer,
			duration: finish
		});
	};
};
