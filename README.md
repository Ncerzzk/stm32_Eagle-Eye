
### STM32_鹰眼摄像头模块

- version 1.0

- introduction

	鹰眼摄像头是一个硬件二值化摄像头，内置感光芯片为OV7725与传统数字摄像头输出YUV或者RGB格式的数据不同，鹰眼摄像头输出的数据
是经过内置比较器的0、1 bit流，也即鹰眼摄像头输出1个字节能代表8个像素点。全速状态下鹰眼输出帧率可达150帧/秒。
由于鹰眼在黑白应用上的高速性，其被广泛应用于飞思卡尔（恩智浦）智能车竞赛的摄像头组。
	
	网上常见的驱动程序都是基于K60，但由于智能车外K60的应用范围有限，本模块使用STM32F407进行驱动。
	
- 驱动原理：
	- 使用场中断 PCLK
	- 场中断信号接外部中断，场中断触发一次，开始一次DMA传输
	- DMA设置为**外设到内存**，并由TIM8 CH4的输入捕捉触发（PCLK）

- Author 
	- [Ncerzzk](https://github.com/Ncerzzk)
	
- View
	- ![view](https://raw.githubusercontent.com/Ncerzzk/stm32_Eagle-Eye/master/view.jpg)
	