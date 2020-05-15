/**
 * @api {POST} /system/login 1.01 登录
 * @apiGroup 1 System
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {String}   userName        用户名
 * @apiParam {String}   passWord        密码,用户输入后,先hash,再传输
 * @apiParam {String}   token           服务器端生成的token,每个命令都需要在http头部的Authorization字段携带token
 * @apiParam {String}   validTime       有效时间,单位:分钟,客户端需要在这个时间内重复登录获取token
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "userName":"admin",
 *                              "passWord":"xxxxxxxx"
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{"token":"xxxxxxx","validTime":30}}
 */

/**
 * @api {POST} /system/logout 1.02 退出
 * @apiGroup 1 System
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {String}   userName        用户名
 * @apiParam {String}   comment         需要携带token
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "userName":"admin"
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

/**
 * @api {POST} /system/init 1.03 系统初始化
 * @apiGroup 1 System
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {String}   masterIp            master机器IP地址
 * @apiParam {String}   msgOutParams        输出消息参数，支持多个消息服务器
 * @apiParam {String}   type                输出消息类型，默认为RabbitMQ
 * @apiParam {String}   host                服务器IP地址
 * @apiParam {int}      port                端口
 * @apiParam {String}   userName            用户名
 * @apiParam {String}   passWord            密码
 * @apiParam {String}   exchange
 * @apiParam {String}   routingKey
 * @apiParam {String}   [gb28181Params]
 * @apiParam {String}   [localHostIp]       本地上级域Host IP地址，可为任意一个slave
 * @apiParam {String}   [localSipId]        本地上级域sipId
 * @apiParam {int}      [localPort]         本地上级域端口
 * @apiParam {String}   [gat1400Params]
 * @apiParam {String}   [localGatHostIp]    本地Host IP地址，可为任意一个slave
 * @apiParam {String}   [localGatServerId]  本地服务Id
 * @apiParam {int}      [localGatPort]      本地服务端口
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "masterIp":"192.168.0.100",
 *                              "msgOutParams":[
 *                                  {
 *                                      "type":"mq",
 *                                      "host":"192.168.0.10",
 *                                      "port":5672,
 *                                      "userName":"guest",
 *                                      "passWord":"guest",
 *                                      "exchange":"aiotc.exchange.message",
 *                                      "routingKey":""
 *                                  }
 *                              ],
 *                              "gb28181Params":{
 *                                  "localHostIp":"192.168.0.99",
 *                                  "localSipId":"34010000002000000001",
 *                                  "localPort":5060
 *                              },
 *                              "gat1400Params":{
 *                                  "localGatHostIp":"34010000002000000001",
 *                                  "localGatServerId":"32020200002000000002",
 *                                  "localGatPort":7100
 *                              }
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

/**
 * @api {POST} /system/slave/add 1.04 添加slave
 * @apiGroup 1 System
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {String}   slaveIp         slave机器IP地址
 * @apiParam {int}      restPort        restAPI服务端口
 * @apiParam {int}      streamPort      socket数据端口,可作为云服务端口
 * @apiParam {String}   [internetIp]    互联网部署时的公网IP地址
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "slaveIp":"192.168.0.100",
 *                              "restPort":11762,
 *                              "streamPort":11764,
 *                              "internetIp":"8.8.8.8"
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

/**
 * @api {POST} /system/slave/del 1.05 删除slave
 * @apiGroup 1 System
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {String}   slaveIp         slave机器IP地址
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "slaveIp":"192.168.0.100"
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {POST} /obj/add/tcp 2.01 添加设备 - tcp
 * @apiGroup 2 Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {String}   [name]          设备名称
 * @apiParam {String}   type            设备类型
 * @apiParam {int}      [id]            设备ID,master支持自动分配
 * @apiParam {String}   subType         数据类型,tcp类型可以支持局域网设备接入互联网云平台
 * @apiParam {String}   source          tcp接入可以兼容多种类型，典型如streamSdk
 * @apiParam {String}   sn              设备序列号
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "name":"test",
 *                              "type":"camera",
 *                              "id":99,
 *                              "data":{
 *                                  "subType":"tcp",
 *                                  "source": "streamSdk",
 *                                  "sn":"12345678"
 *                              }
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccess (200) {int}      id      设备ID
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{"id":99}}
 */

 /**
 * @api {POST} /obj/add/ehome 2.02 添加设备 - Ehome
 * @apiGroup 2 Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {String}   [name]          设备名称
 * @apiParam {String}   type            设备类型
 * @apiParam {int}      [id]            设备ID,master支持自动分配
 * @apiParam {String}   subType         数据类型,Ehome为海康设备接入互联网协议
 * @apiParam {String}   deviceId        设备ID
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "name":"test",
 *                              "type":"camera",
 *                              "id":99,
 *                              "data":{
 *                                  "subType":"ehome",
 *                                  "deviceId":"12345678"
 *                              }
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccess (200) {int}      id      设备ID
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{"id":99}}
 */

 /**
 * @api {POST} /obj/add/gat1400 2.03 添加设备 - gat1400
 * @apiGroup 2 Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {String}   [name]          设备名称
 * @apiParam {String}   type            设备类型
 * @apiParam {int}      [id]            设备ID,master支持自动分配
 * @apiParam {String}   subType         数据类型
 * @apiParam {String}   deviceId        设备Id，一般与某个像机绑定
 * @apiParam {int}      mode            0:推送模式 1:订阅模式
 * @apiParam {String}   [slaveIp]       推送模式时需要指定服务器IP
 * @apiParam {String}   hostServerId    对方服务器ID
 * @apiParam {String}   hostIp          对方服务器IP地址
 * @apiParam {int}      hostPort        对方服务器端口
 * @apiParam {String}   [userName]      对方服务器登录用户名,订阅模式
 * @apiParam {String}   [passWord]      对方服务器登录密码,订阅模式
 * @apiParam {String}   [extra]         扩展字段
 * @apiParam {String}   [manufactor]    厂家名字，用于区分各个厂家的非标协议
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "name":"test",
 *                              "type":"camera",
 *                              "id":99,
 *                              "data":{
 *                                  "subType":"gat1400",
 *                                  "deviceId": "32020200002000000003",
 *                                  "mode": 0,
 *                                  "slaveIp": "192.168.0.100",
 *                                  "hostServerId": "32020200002000000001",
 *                                  "hostIp": "192.168.0.64",
 *                                  "hostPort": 5060,
 *                                  "userName": "admin",
 *                                  "passWord": "admin",
 *                                  "extra": {
 *                                      "manufactor": "hik"
 *                                  }
 *                              }
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccess (200) {int}      id      设备ID
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{"id":99}}
 */

 /**
 * @api {POST} /obj/add/rtsp 2.04 添加设备 - rtsp
 * @apiGroup 2 Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {String}   [name]          设备名称
 * @apiParam {String}   type            设备类型
 * @apiParam {int}      [id]            设备ID,master支持自动分配
 * @apiParam {String}   subType         数据类型
 * @apiParam {int}      tcpEnable       rtsp模式下的tcp使能, 1:tcp,0:udp
 * @apiParam {String}   url             rtsp视频流地址
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "name":"test",
 *                              "type":"camera",
 *                              "id":99,
 *                              "data":{
 *                                  "subType":"rtsp",
 *                                  "tcpEnable":0,
 *                                  "url":"rtsp://192.168.0.64/h264/ch1/main/av_stream"
 *                              }
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccess (200) {int}      id      设备ID
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{"id":99}}
 */

 /**
 * @api {POST} /obj/add/gb28181 2.05 添加设备 - gb28181
 * @apiGroup 2 Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {String}   [name]          设备名称
 * @apiParam {String}   type            设备类型
 * @apiParam {int}      [id]            设备ID,master支持自动分配
 * @apiParam {String}   subType         数据类型
 * @apiParam {int}      tcpEnable       gb28181模式下的tcp使能, 1:tcp,0:udp
 * @apiParam {String}   sipId           视频流sipId，一般与某个像机绑定
 * @apiParam {String}   hostSipId       远程下级域sipId，可以同时对接多个下级域服务器
 * @apiParam {String}   hostIp          远程下级域IP地址
 * @apiParam {int}      hostSipPort     远程下级域端口
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "name":"test",
 *                              "type":"camera",
 *                              "id":99,
 *                              "data":{
 *                                  "subType":"gb28181",
 *                                  "tcpEnable":0,
 *                                  "sipId": "32020200002000000003",
 *                                  "hostSipId": "32020200002000000001",
 *                                  "hostIp": "192.168.0.64",
 *                                  "hostSipPort": 5060
 *                              }
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccess (200) {int}      id      设备ID
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{"id":99}}
 */

 /**
 * @api {POST} /obj/add/sdk 2.06 添加设备 - 厂家sdk
 * @apiGroup 2 Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {String}   [name]          设备名称
 * @apiParam {String}   type            设备类型
 * @apiParam {int}      [id]            设备ID,master支持自动分配
 * @apiParam {String}   subType         像机类型,海康,大华等各厂家通过subType不同可以共用此接口,目前支持hikSdk/dhSdk
 * @apiParam {String}   ip              IP地址
 * @apiParam {int}      port            端口
 * @apiParam {String}   userName        用户名
 * @apiParam {String}   passWord        密码
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "name":"test",
 *                              "type":"camera",
 *                              "id":99,
 *                              "data":{
 *                                  "subType":"hikSdk",
 *                                  "ip":"192.168.0.64",
 *                                  "port":8000,
 *                                  "userName":"admin",
 *                                  "passWord":"admin"
 *                              }
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccess (200) {int}      id      设备ID
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{"id":99}}
 */

 /**
 * @api {POST} /obj/stream/start 2.07 开始拉流
 * @apiGroup 2 Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {int}      id              设备ID
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id":99
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {POST} /obj/stream/stop 2.08 停止拉流
 * @apiGroup 2 Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {int}      id              设备ID
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id":99
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {POST} /obj/preview/start 2.09 开始视频预览
 * @apiGroup 2 Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam    {int}       id          设备ID
 * @apiParam    {String}    type        预览模式,支持hls,http-flv,ws等
 * @apiParam    {String}    comment     设备类型为camera,才支持视频预览,此命令依赖/obj/stream/start
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id": 99,
 *                              "type": "hls"
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{"url":"http://192.168.0.10:8080/m3u8/stream9/play.m3u8"}}
 */

 /**
 * @api {POST} /obj/preview/stop 2.10 停止视频预览
 * @apiGroup 2 Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {int}      id              设备ID
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id":99
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {POST} /obj/record/start 2.11 开始录像
 * @apiGroup 2 Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam    {int}       id          设备ID
 * @apiParam    {String}    comment     设备类型为camera,才支持视频录像,此命令依赖/obj/stream/start
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id":99
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {POST} /obj/record/stop 2.12 停止录像
 * @apiGroup 2 Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {int}      id              设备ID
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id":99
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {POST} /obj/record/play 2.13 录像回放
 * @apiGroup 2 Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {int}      id                  设备ID
 * @apiParam {int}      startTimeStamp      开始时间
 * @apiParam {int}      stopTimeStamp       结束时间
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id":99,
 *                              "startTimeStamp":1582201220,
 *                              "stopTimeStamp":1582221220
 *                          }
 * @apiSuccess (200) {int}      code        0:成功 1:失败
 * @apiSuccess (200) {String}   msg         信息
 * @apiSuccess (200) {String}   type        录像回放类型，一般为MP4，也可能为其他，比如hls
 * @apiSuccess (200) {String}   playUrl     播放列表
 * @apiSuccess (200) {int}      [index]     只在type为mp4时有效，索引号
 * @apiSuccess (200) {int}      [start]     只在type为mp4时有效，表示此文件开始时间
 * @apiSuccess (200) {int}      [len]       只在type为mp4时有效，表示文件时间长度
 * @apiSuccess (200) {String}   url         播放地址，根据type不同，客户端需要选择不同播放方案
 * @apiSuccessExample {json} 返回样例:
 *                          {
 *                              "code":0,
 *                              "msg":"success",
 *                              "data":{
 *                                  "type":"mp4",
 *                                  "playUrl":[
 *                                      {
 *                                          "index": 1,
 *                                          "start": 1582201220,
 *                                          "len": 600,
 *                                          "url": "http://xxxx:xxxx/xxxx.mp4"
 *                                      },
 *                                      {
 *                                          "index": 2,
 *                                          "start": 1582201820,
 *                                          "len": 600,
 *                                          "url": "http://xxxx:xxxx/xxxx.mp4"
 *                                      }
 *                                  ]
 *                              }
 *                          }
 */

 /**
 * @api {POST} /obj/capture/start 2.14 开始抓拍
 * @apiGroup 2 Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam    {int}       id          设备ID
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id":99
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {POST} /obj/capture/stop 2.15 停止抓拍
 * @apiGroup 2 Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {int}      id              设备ID
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id":99
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {POST} /obj/del 2.16 删除设备
 * @apiGroup 2 Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam {int}      id              设备ID
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id":99
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {POST} /task/start 3.01 开始任务
 * @apiGroup 3 Task
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam    {int}       id          设备ID
 * @apiParam    {String}    alg         每个设备可以启动多个算法，详见[算法支持]接口
 * @apiParam    {String}    comment     此命令依赖/obj/stream/start或/obj/capture/start
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id":999,
 *                              "alg":"faceCapture"
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {POST} /task/stop 3.02 停止任务
 * @apiGroup 3 Task
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam    {int}       id          设备ID
 * @apiParam    {String}    alg         算法名称
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id":999,
 *                              "alg":"faceCapture"
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {GET} /alg/support 4.01 算法支持
 * @apiGroup 4 Alg
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiSuccess (200) {int}      code            0:成功 1:失败
 * @apiSuccess (200) {String}   msg             信息
 * @apiSuccess (200) {String}   alg             支持的算法种类
 * @apiSuccess (200) {String}   faceCapture     人脸抓拍,只适合视频流输入设备
 * @apiSuccess (200) {String}   vehCapture      车辆抓拍,只适合视频流输入设备
 * @apiSuccess (200) {String}   personAttr      行人属性提取,年龄、性别、眼镜、衣服颜色等,输入为抓拍图片
 * @apiSuccess (200) {String}   vehAttr         车辆属性提取,车型、颜色等,输入为抓拍图片
 * @apiSuccess (200) {String}   plateRecg       车牌识别,输入为视频流或抓拍图片
 * @apiSuccess (200) {String}   yolo            开源算法集成
 * @apiSuccess (200) {String}   openpose        开源算法集成
 * @apiSuccessExample {json} 返回样例:
 *                          {
 *                              "code":0,
 *                              "msg":"success",
 *                              "data":{
 *                                  "alg":["faceCapture","vehCapture","personAttr","vehAttr","plateRecg","yolo","openpose"]
 *                              }
 *                          }
 */

 /**
 * @api {OUT} /common 5.01 任务结果消息
 * @apiGroup 5 Output
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiSuccessExample {json} 输出样例:
 *                          {
 *                              "msgType": "common",
 *                              "data":{
 *                                  "id":999,
 *                                  "timeStamp": 99999999,
 *                                  "sceneImg": {
 *                                      "url": "http://xxxx:xxxx/scene.jpg"
 *                                  },
 *                                  "person":[{
 *                                      "face":{
 *                                          "url": "http://xxxx:xxxx/face.jpg"
 *                                          "rect": {"x":0.001,"y":0.001,"w":0.012,"h":0.033}
 *                                          "quality": 0.9
 *                                      },
 *                                      "body":{
 *                                          "url": "http://xxxx:xxxx/body.jpg"
 *                                          "rect": {"x":0.001,"y":0.001,"w":0.012,"h":0.033}
 *                                      },
 *                                      "property":{
 *                                          "gender":{"value":"man","confidence":99.99},
 *                                          "age":{"value":33,"confidence":99.99}
 *                                      }
 *                                  }],
 *                                  "veh":[{
 *                                      "plate":{
 *                                          "color":"blue",
 *                                          "plateNo":"苏A99999",
 *                                          "url": "http://xxxx:xxxx/plate.jpg"
 *                                          "rect": {"x":0.001,"y":0.001,"w":0.012,"h":0.033},
 *                                      },
 *                                      "body":{
 *                                          "url": "http://xxxx:xxxx/body.jpg"
 *                                          "rect": {"x":0.001,"y":0.001,"w":0.012,"h":0.033}
 *                                      },
 *                                      "property":{
 *                                          "color":{"value":"blue","confidence":99.99},
 *                                          "brand":{"value":"宝马-宝马X5-2014","confidence":99.99}
 *                                      }
 *                                  }]
 *                              }
 *                          }
 */

 /**
 * @api {OUT} /status 5.02 设备状态消息
 * @apiGroup 5 Output
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiParam    {int}       id          设备ID
 * @apiParam    {int}       status      1:在线,0:离线
 * @apiParam    {int}       stream      1:开始拉流,0:停止拉流
 * @apiParam    {int}       preview     1:开始预览,0:停止预览
 * @apiParam    {int}       capture     1:开始抓拍,0:停止抓拍
 * @apiParam    {int}       record      1:开启录像,0:停止录像
 * @apiSuccessExample {json} 输出样例:
 *                          {
 *                              "msgType": "objStatus",
 *                              "data":{
 *                                  "obj":[
 *                                      {
 *                                          "id": 999,
 *                                          "status" : 1,
 *                                          "stream" : 1,
 *                                          "preview" : 1,
 *                                          "capture" : 1,
 *                                          "record" : 1
 *                                      }
 *                                  ]
 *                              }
 *                          }
 */

