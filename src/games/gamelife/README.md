生命游戏是John Conway 发现的一种游戏。 其底层规则非常简单。 在一个二维点状平面上， 每一个点遵循如下规则

* 少于2个邻居的点，在下一回合死去。模拟生命较少的情况。
* 在周围邻居数量是2和3时，下一回合保持不变
* 在周围邻居数量大于3时，下一回合死去，模拟生命拥挤的情况。
* 当一个空白的点，周围的邻居数量是3个是， 下一回合将会产生一个新的点。模拟繁殖。


视频: https://www.bilibili.com/video/BV13Q4y1K7nS?spm_id_from=333.337.search-card.all.click
代码参考: https://github.com/Creativethink1/Game-of-Life


https://troytae.github.io/game-of-life/
https://playgameoflife.com/


// https://conwaylife.com/wiki/Category:Animated_images
// https://conwaylife.com/w/images/5/50/Circleoffire.gif