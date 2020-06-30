## aiotc
AIoT视频云平台
****

## 文档
[RESTAPI接口](https://debugger999.github.io/aiotc/doc/html/index.html)

## 测试步骤
[链接](https://debugger999.github.io/2020/06/29/js-version)

## 特性
* [接口](#文本)
    * RESTful + json
* [输入](#文本)
    * 支持任何格式数据输入，视频、图片、音频、文本等
    * 第一阶段主要是互联网摄像头接入，包括tcp streamSdk、Ehome、gat1400等
    * 第二阶段支持rtsp、gb28181、hikSdk/dhSdk/ftp等，这些类型主要在局域网或专网运行
* [输出](#文本)
    * RabbitMQ + json
* [算法](#文本)
    * 算法只需要封装为标准的init/start/stop/uninit接口，可以很容易集成到aiotc
    * 任何一个输入设备都可以关联N个算法，比如一路rtsp视频流可以同时运行人脸抓拍、车辆结构化
* [集群](#文本)
    * 支持master/slave模式，自动负载均衡
    * 每个slave支持GPU一机多卡
    * 支持数据库参数持久化，用户只需要调用一次RESTful，重启自动运行任务
* [客户端](#文本)
    * 本项目开发重心在服务器端，高性能、高并发、易扩展等，客户端只是demo级别
    * 第一阶段目标实现浏览器访问
    * 第二阶段实现APP访问
* [其他](#文本)
    * 互联网云平台免费试用
    * 浏览器H5实时预览，支持hls、http-flv、websocket
    * 支持告警录像联动，基于MP4的url输出
    * 考虑支持P2P模式，可以在互联网接入时节省带宽，及提高视频预览实时性

## 场景
![add image](https://github.com/debugger999/aiotc/raw/master/doc/img/aiotc.jpg)

## 开发日记
[主页](https://debugger999.github.io)

## 技术交流QQ群
652045690
