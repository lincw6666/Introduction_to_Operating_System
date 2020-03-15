NCTU OS Homework 03
===
Optimize image processing by using threads and synchronization.
- Part 01: Blur with Gaussian filter.
- Part 02: Edge detection with Sobel filter.

# The Purpose of Those Threads
- 這兩題各有兩⼤部份： 轉成灰階 -> 濾波
- 我用了 N 個 threads，把每⼀個部份切成 N 個 data，⽤ threads 平⾏運算， 算完⼀個部份再換下⼀個部份
- 我⽤ mutexlock 和 semaphore 等 "轉成灰階" 這個部份做完， 然後才做濾波
  - ⚠ (Update) 現在發現這個部分用 lock 的意義不明，待改善

# How I Speed Up My Code
⽤ threads 加速⼤概快 1. 多倍（約670000）就極限了， 所以我用 gprof 分析程式各個 function 總共⽤的時間、被呼叫的次數和每次呼叫所花的時間，發現濾波花的時間最久，⽽且每次呼叫花的時間很多

![](https://i.imgur.com/CrVUvkA.png)

原本是要用 unrolling 來加速，但後來得知 filter 的 size 會改變， 所以就不能用了QQ 我現在做的只是減少濾波函式裡加減乘除的次數

⚠ (Update) 現在想想就算 filter 的 size 會改變，還是可以用 unrolling 啊 = =

# Results
Tested on a two threads CPU.
- 第1題： 359504
- 第2題： 325117

![](https://i.imgur.com/am7Blkl.png)
