# part1
view1 :
~~~txt
[mandelbrot serial]:            [217.952] ms
Wrote image file mandelbrot-serial.ppm
[mandelbrot ispc]:              [44.823] ms
Wrote image file mandelbrot-ispc.ppm
                                (4.86x speedup from ISPC)
~~~

view2 :
~~~txt
[mandelbrot serial]:            [134.371] ms
Wrote image file mandelbrot-serial.ppm
[mandelbrot ispc]:              [32.859] ms
Wrote image file mandelbrot-ispc.ppm
                                (4.09x speedup from ISPC)
~~~

Logically, I can acheieve 8× speedup since I use 8with AVX2 vector instructions. But both pictures's speedup only achive approximately 4× speedup because of the load unbalance of 8 lanes leading to divergengce. Compare the two pictures, I could see view2 achieve even poorer speedup, because view2 has more black white transition parts leading to more [divergences](https://ispc.github.io/ispc.html#basic-concepts-program-instances-and-gangs-of-program-instances:~:text=an%20Intel%20GPU.-,Control%20Flow%20Within%20A%20Gang,-Almost%20all%20the).

# Part2

| n | serial (ms) | ispc (ms) | task (ms) | ispc speedup | task speedup |
|---|-------------|-----------|-----------|--------------|--------------|
| 2 | 231.372 | 47.954 | 24.061 | 4.824874 | 9.616059 |
| 4 | 233.221 | 47.006 | 19.038 | 4.961516 | 12.250289 |
| 8 | 231.746 | 48.002 | 11.902 | 4.827841 | 19.471181 |
| 16 | 230.771 | 48.183 | 7.736 | 4.789469 | 29.830791 |
| 32 | 232.542 | 47.856 | 6.499 | 4.859203 | 35.781197 |
| 80 | 230.633 | 47.435 | 7.314 | 4.862085 | 31.533087 |
| 160 | 230.611 | 47.859 | 7.477 | 4.818550 | 30.842718 |

Theoretically, only 8 threads may achieve the best performance because my lab environment has 4core,8hyperthreadings. But pratically I achieve the best performance when using 32 tasks where we may distribute the computation tasks more evenly even though this may lead to more contextswtich.
