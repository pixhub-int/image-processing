# butteraugli wasm

Инструмент для определения степени различия изображений, работающий в браузере.  
[github.com/google/butteraugli](https://github.com/google/butteraugli)

Посмотреть в работе можно здесь  
https://pixhub-int.github.io/image-processing/butteraugli/

---

`butteraugli` уже собирали на `wasm`.
Отличие в том, что исходник был лишь у версии, переписанной на чистый джс. Она медленней и основана на устаревшей версии алгоритма.  
[libwebpjs.hohenlimburg.org/butteraugli](http://libwebpjs.hohenlimburg.org/butteraugli/)  
[github.com/dominikhlbg/butteraugli.js/issues/1](https://github.com/dominikhlbg/butteraugli.js/issues/1)

Версия от Доминика @dominikhlbg рядом  
https://pixhub-int.github.io/image-processing/butteraugli/dominikhlbg/

Сборке помогла статья.
В частности, использовался докер с предустановленным эмскриптеном.  
[developers.google.com/web/updates/2018/03/emscripting-a-c-library](https://developers.google.com/web/updates/2018/03/emscripting-a-c-library)

Собиралась из версии от 22 июня 2018.  
[github.com/google/butteraugli/commit/832436e2e1b8ada1dff89825716139768511c3c8](https://github.com/google/butteraugli/commit/832436e2e1b8ada1dff89825716139768511c3c8)

Вместе с исходниками вы можете найти соответствующие файлы `.diff`.

Были выполнены следующие изменения.  
— Убраны `libpng` и `libjpeg` из сборки, на вход сразу будет отправляться `RGBA` в `uint8_t`.  
— Карта отличий сразу генерируется также в формате `RGBA` в `uint8_t`.  
— Была применена оптимизация из запроса на изменение, так как те же изменения были применены для Pik  
[github.com/google/butteraugli/pull/49](https://github.com/google/butteraugli/pull/49)  
[github.com/google/pik/commit/6fa4a5b96131d5b251d588535452b1cfb377d8e3#diff-bfdc5256f2a4ef0db5ead0c06cdbb126](https://github.com/google/pik/commit/6fa4a5b96131d5b251d588535452b1cfb377d8e3#diff-bfdc5256f2a4ef0db5ead0c06cdbb126)

Сборка выполнялась следующими командами.

```
> docker run --rm -v $(pwd):/src trzeci/emscripten emmake make
> sudo docker run --rm -v $(pwd):/src trzeci/emscripten emcc butteraugli.o butteraugli_main.o -Oz -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS='["_butteraugli_output", "_mfree"]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["cwrap"]' -s NO_FILESYSTEM=1 -o butteraugli.html
> python server.py
```
