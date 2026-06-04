## implementA:
launch threads whose num is equal to image size, then in each thread iterate through all circles sequentially and compute the color.

haness test:
~~~md
Score table:
------------
--------------------------------------------------------------------------
| Scene Name      | Ref Time (T_ref) | Your Time (T)   | Score           |
--------------------------------------------------------------------------
| rgb             | 0.6776           | 0.6486          | 9               |
| rand10k         | 4.9914           | 86.3985         | 2               |
| rand100k        | 48.5121          | 926.3378        | 2               |
| pattern         | 1.1247           | 10.2022         | 3               |
| snowsingle      | 30.4144          | 814.5684        | 2               |
| biglittle       | 27.888           | 100.6485        | 4               |
| rand1M          | 377.6627         | 9409.3996       | 2               |
| micro2M         | 706.2951         | 19775.2363      | 2               |
--------------------------------------------------------------------------
|                                    | Total score:    | 26/72           |
--------------------------------------------------------------------------
~~~