This folder contains a specific library to manage filterbanks of a generic sigmastudio program and combine them as preset.
The program will generate all the biquad coefficient and the downloadable parameter table.
there are also couple of routines to calculate filter response and provide convenient tables for onscreen drawing
this V5 considers one filter bank matrix of N biquad for each input and each output, and 2 additional banks for HighPass and LowPass filter of 4 biquad each, positioned before the main filter bank. this way a concept of crossover and then EQ might be easier to implement.
Also this V5 support special feedback and delay to implement delayed substractive crossover.
