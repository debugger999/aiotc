/******************************************************************************
 * Copyright (C) 2019-2025 debugger999 <debugger999@163.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#ifndef __AIOTC_GAT1400SDK_H__
#define __AIOTC_GAT1400SDK_H__

#ifdef __cplusplus
extern "C" {
#endif

#define GA1400_REQUEST_URL "/api/v1/ga1400"

enum GAT1400_DATATYPE {
    GAT1400_FACE,
    GAT1400_PERSON,
    GAT1400_VEH,
    GAT1400_NONMOTOR
};

typedef char dateTime[20];
typedef char CaseObjectIdType[32];                      //案件管理对象id
typedef char BusinessObjectIdType[36];                  //布控，订阅，通知对象id
typedef char BasicObjectIdType[64];                     //图像信息基本要素ID，视频、图像、文件
typedef char ImageCntObjectIdType[64];                  //图像信息内容要素ID，人、人脸、机动车、非机动车、物品、场景等
typedef char PlaceFullAddressType[128];                 //具体到摄像机位置或街道门牌号
typedef char DeviceIDType[64];                          //采集设备、卡口点位、采集系统、分析系统、视图库、应用平台等设备编码规则
typedef char EscapedCriminalNumberType[32];             //在逃人员信息编号规则
typedef char DetentionHouseCodeType[16];                //看守所编码
typedef char SuspectedTerroristNumberType[24];          //涉恐人员编号
typedef char IDType_[4];                                //常用证件代码
typedef char EthicCodeType[4];                          //中国各名族的罗马字母拼写法和代码
typedef char NationalityCodeType[4];                    //国籍代码、世界各国和地区名称代码
typedef char ChineseAccentCodeType[8];                  //汉语口音编码规则
typedef char JobCategoryType[4];                        //职业分类与代码，不包含代码中“—”
typedef char BodyType_[4];                              //体型
typedef char SkinColorType[4];                          //肤色
typedef char FaceStyleType[8];                          //脸型
typedef char FacialFeatureType[64];                     //脸部特征
typedef char PhysicalFeatureType[256];                  //体貌特征
typedef char BodyFeatureType[80];                       //体表特征，有多个时用英文半角逗号分隔
typedef char ImmigrantTypeCodeType[4];                  //出入境人员分类代码
typedef char CriminalInvolvedSpecilisationCodeType[4];  //涉恐人员专长代码
typedef char BodySpeciallMarkType[8];                   //体表特殊标记
typedef char CrimeMethodType[8];                        //作案手段
typedef char CrimeCharacterCodeType[4];                 //作案特点分类和代码
typedef char DetaineesSpecialIdentityType[4];           //在押人员特殊身份代码
typedef char MemberTypeCodeType[4];                     //成员类型代码
typedef char VictimType[4];                             //被害人类型
typedef char CorpseConditionCodeType[4];                //尸体状况分类与代码
typedef char DrivingStatusCodeType[8];                  //机动车行驶状态代码
typedef int UsingPropertiesCodeType;                    //机动车使用性质代码
typedef char WheelPrintedPatternType[4];                //车轮印花纹
typedef char PlaceType[8];                              //选择处所代码
typedef char WeatherType[4];                            //天气代码
typedef char SceneType[4];                              //道路类型代码
typedef char RoadAlignmentType[4];                      //道路线形代码
typedef char RoadSurfaceType[4];                        //道路路面类型代码
typedef char RoadCoditionType[4];                       //道路路面状况代码
typedef char RoadJunctionSectionType[4];                //道路路口路段类型代码
typedef char WindDirectionType[4];                      //现场风向代码
typedef char InvolvedObjType[8];                        //涉案物品分类和代码
typedef char FirearmsAmmunitionType[4];                 //枪支弹药类别
typedef char ToolTraceType[4];                          //工具痕迹分类和代码
typedef char EvidenceType[8];                           //物证类别
typedef char CaseEvidenceType[4];                       //案(事)件物证形态代码
typedef char PlaceCodeType[8];                          //行政区划、籍贯省市县代码
typedef char IPAddrType[32];                            //IP地址
typedef char NameType[64];                              //姓名
typedef char UsedNameType[64];                          //曾用名
typedef char AliasType[64];                             //别名、绰号
typedef char OrgType[128];                              //单位名称
typedef char FileNameType[256];                         //电子文件名
typedef char IdNumberType[32];                          //证件号码
typedef char StatusType[4];                             //视频设备工作状态:1在线、2离线、9其他
typedef char PlateNoType[24];                           //机动车号牌号码
typedef char CaseNameType[128];                         //案件名称
typedef char CaseAbstractType[4096];                    //简要案情
typedef char DeviceNameType[128];                       //设备名称
typedef char CaseLinkMarkType[32];                      //案件编号
typedef char PasswordType[36];                          //口令
typedef double LongitudeType;                           //地球经度n10，6;精确到小数点后6位
typedef double LatitudeType;                            //地球纬度n10，6;精确到小数点后6位
typedef char KeywordType[256];                          //关键词
typedef char ModelType[128];                            //设备型号
typedef char VehicleModelType[36];                      //车辆型号
typedef char OrgCodeType[16];                           //机构代码
typedef char IPV6AddrType[64];                          //IPv6地址
typedef int NetPortType;                                //网络端口号
typedef char TollgateType[4];                           //卡口类型10，国际; 20，省际; 30，市际; 31，市区; 40，县际; 41，县区;  99，其他;
typedef char VehicleClassType[4];                       //机动车车辆类型代码
typedef double SpeedType;                               //
typedef int VehicleLengthType;                          //5位整数，单位为毫米（mm）
typedef int VehicleWidthType;                           //4位整数，单位为毫米（mm）
typedef int VehicleHeightType;                          //4位整数，单位为毫米（mm）
typedef char DataSourceType[4];                         //视频图像数据来源
typedef char AudioCodeFormatType[4];                    //音频编码格式
typedef char VideoCodeFormatType[4];                    //视频编码格式
typedef char ColorType[4];                              //颜色
typedef char SecretLevelType[4];                        //密级代码
typedef char QualityGradeType[4];                       //质量等级
typedef char HorizontalShotType[4];                     //水平拍摄方向
typedef char VerticalShotType[4];                       //垂直拍摄方向
typedef char HairStyleType[4];                          //发型
typedef char PostureType[4];                            //姿态
typedef char PersonStatusType[4];                       //人的状态
typedef char HabitualActionType[4];                     //习惯动作
typedef char BehaviorType[4];                           //行为
typedef char AppendageType[4];                          //附属物
typedef char HatStyleType[4];                           //帽子款式
typedef char GlassesStyleType[4];                       //眼镜款式
typedef char BagStyleType[4];                           //包款式
typedef char CoatStyleType[4];                          //上衣款式
typedef char CoatLengthType[4];                         //上衣长度
typedef char PantsStyleType[4];                         //裤子款式
typedef char PantsLengthType[4];                        //裤子长度
typedef char ShoesStyleType[4];                         //鞋子款式
typedef char Boolean[4];                                //二值逻辑
typedef char AutoFoilColorType[4];                      //贴膜颜色
typedef char DentInfoType[4];                           //撞痕信息
typedef char FrontThingType[64];                        //车前部物品类型
typedef char RearThingType[64];                         //车后部物品类型
typedef char ThingPropertyType[4];                      //物品性质
typedef char IllustrationType[4];                       //现场图示
typedef char IlluminationType[4];                       //现场光线
typedef char FieldConditionType[4];                     //现场条件
typedef char HumidityType[4];                           //现场湿度
typedef char DenseDegreeType[4];                        //密集程度
typedef char InjuredDegreeType[4];                      //受伤害程度
typedef char RoadTerraintype[4];                        //道路地形分类
typedef char DetaineesIdentityType[4];                  //在押人员身份
typedef char GenderType[4];                             //性别
typedef char PlateClassType[4];                         //号牌种类
typedef char enPassportType[4];                         //护照证件种类
typedef char VideoFormatType[8];                        //视频格式
typedef char ImageFormatType[8];                        //图片格式
typedef char VehicleBrandType[4];                       //车辆品牌代码
typedef char DispositionRangeType[4];                   //布控范围
typedef int InfoType;                                   //视频图像信息类型
typedef char ConfirmStatusType[4];                      //确认状态
typedef char DispositionCategoryType[4];                //布控类别
typedef int PropertyType;                               //布控对象名称中属性类型
typedef char SubscribeDetailType[4];                    //订阅类别
typedef char TimeCorrectModeType[4];                    //校时模式
typedef int EventType;                                  //视频图像分析处理事件类型
typedef char VehicleColorDepthType[4];                  //颜色深浅
typedef char HDirectionType[4];                         //水平方向

typedef struct nodecommon1400 {
    char name[256];
    int val;
    void* arg;
    struct nodecommon1400* next;
} node_common_1400;

typedef struct {
    node_common_1400* head;
    node_common_1400* tail;
    int queLen;
} queue_common_1400;

typedef struct {
    BasicObjectIdType ImageID;   //图像标识
    EventType EventSort;         //事件分类
    char StoragePath[256];       //存储路径
    DeviceIDType DeviceID;       //设备编码
    char Type[12];               //图像类型
    ImageFormatType FileFormat;  //图像文件格式
    dateTime ShotTime;           //抓拍时间
    int FileSize;                //文件大小
    int Width;                   //图片宽
    int Height;                  //图片高
    char* Data;                  //图像数据
} SubImageInfo;

typedef struct {
    ImageCntObjectIdType FaceID;              //人脸标识
    InfoType InfoKind;                        //信息分类
    BasicObjectIdType SourceID;               //来源标识
    DeviceIDType DeviceID;                    //设备编码
    int LeftTopX;                             //人脸左上角右下角坐标
    int LeftTopY;                             //左上
    int RightBtmX;                            //右下
    int RightBtmY;                            //右下
    dateTime ShotTime;                        //抓拍时间
    int IsDriver;                             //是否驾驶员
    int IsForeigner;                          //是否涉外人员
    int IsSuspectedTerrorist;                 //是否涉恐人员
    int IsCriminalInvolved;                   //是否涉案人员
    int IsDetainees;                          //是否在押人员
    int IsSuspiciousPerson;                   //是否可疑人
    int IsVictim;                             //是否被害人
    dateTime LocationMarkTime;                //人工采集位置标记时间
    dateTime FaceAppearTime;                  //人工采集人脸出现时间
    dateTime FaceDisAppearTime;               //人工采集人脸消失时间
    IDType_ IDType;                           //证件类型
    IdNumberType IDNumber;                    //证件号码
    NameType Name;                            //中文姓名全称
    UsedNameType usedName;                    //曾用名
    AliasType Alias;                          //绰号
    GenderType GenderCode;                    //性别
    int AgeUpLimit;                           //最大可能年龄
    int AgeLowerLimit;                        //最小可能年龄
    EthicCodeType EthicCode;                  //民族代码
    NationalityCodeType NationalityCode;      //国籍代码
    PlaceCodeType NativeCityCode;             //籍贯市县代码
    PlaceCodeType ResidenceAdminDivision;     //居住的行政规划
    ChineseAccentCodeType ChineseAccentCode;  //汉语口音代码
    JobCategoryType JobCategory;              //职业分类代码
    int AccompanyNumber;                      //同行人脸数
    SkinColorType SkinColor;                  //肤色
    HairStyleType HairStyle;                  //发型
    ColorType HairColor;                      //发色
    FaceStyleType FaceStyle;                  //脸型
    FacialFeatureType FacialFeature;          //脸部特征
    PhysicalFeatureType PhysicalFeature;      //体貌特征
    ColorType RespiratorColor;                //口罩颜色
    HatStyleType CapStyle;                    //帽子款式
    ColorType CapColor;                       //帽子颜色
    GlassesStyleType GlassStyle;              //眼镜款式
    ColorType GlassColor;                     //眼镜颜色
    enPassportType PassportType;              //护照证件种类
    ImmigrantTypeCodeType ImmigrantTypeCode;  //出入境人员类别
    BodySpeciallMarkType BodySpeciallMark;    //体表特殊标记
    int Attitude;                             //姿态分布
    double Similaritydegree;                  //人脸相似度
    char EyebrowStyle[32];                    //眉形
    char NoseStyle[32];                       //鼻形
    char MustacheStyle[32];                   //胡形
    char LipStyle[32];                        //嘴唇
    char WrinklePouch[32];                    //皱纹眼袋
    char AcneStain[32];                       //痤疮色斑
    char FreckleBirthmark[32];                //黑痣胎记
    char ScarDimple[32];                      //疤痕酒窝
    char OtherFeature[32];                    //其他特征
    queue_common_1400 SubImageList;           //图像列表
} Face;

typedef struct {
    ImageCntObjectIdType PersonID;            //人脸标识
    InfoType InfoKind;                        //信息分类
    BasicObjectIdType SourceID;               //来源标识
    DeviceIDType DeviceID;                    //设备编码
    int LeftTopX;                             //人员左上角右下角坐标
    int LeftTopY;                             //左上
    int RightBtmX;                            //右下
    int RightBtmY;                            //右下
    dateTime ShotTime;                        //抓拍时间
    int IsDriver;                             //是否驾驶员
    int IsForeigner;                          //是否涉外人员
    int IsSuspectedTerrorist;                 //是否涉恐人员
    int IsCriminalInvolved;                   //是否涉案人员
    int IsDetainees;                          //是否在押人员
    int IsSuspiciousPerson;                   //是否可疑人
    int IsVictim;                             //是否被害人
    enPassportType PassportType;              //护照证件种类
    ImmigrantTypeCodeType ImmigrantTypeCode;  //出入境人员类别
    BodySpeciallMarkType BodySpeciallMark;    //体表特殊标记
    int Attitude;                             //姿态分布
    dateTime LocationMarkTime;                //人工采集位置标记时间
    dateTime PersonAppearTime;                //人工采集人脸出现时间
    dateTime PersonDisAppearTime;             //人工采集人脸消失时间
    IDType_ IDType;                           //证件类型
    IdNumberType IDNumber;                    //证件号码
    NameType Name;                            //中文姓名全称
    UsedNameType usedName;                    //曾用名
    AliasType Alias;                          //绰号
    GenderType GenderCode;                    //性别
    int AgeUpLimit;                           //最大可能年龄
    int AgeLowerLimit;                        //最小可能年龄
    EthicCodeType EthicCode;                  //民族代码
    NationalityCodeType NationalityCode;      //国籍代码
    PlaceCodeType NativeCityCode;             //籍贯市县代码
    PlaceCodeType ResidenceAdminDivision;     //居住的行政规划
    ChineseAccentCodeType ChineseAccentCode;  //汉语口音代码
    OrgType PersonOrg;                        //工作单位
    JobCategoryType JobCategory;              //职业分类代码
    int AccompanyNumber;                      //同行人数
    int HeightUpLimit;                        //身高上限
    int HeightLowerLimit;                     //身高下限
    BodyType_ BodyType;                       //体型
    SkinColorType SkinColor;                  //肤色
    HairStyleType HairStyle;                  //发型
    ColorType HairColor;                      //发色
    PostureType Gesture;                      //姿态
    PersonStatusType Status;                  //状态
    FaceStyleType FaceStyle;                  //脸型
    FacialFeatureType FacialFeature;          //脸部特征
    PhysicalFeatureType PhysicalFeature;      //体貌特征
    BodyFeatureType BodyFeature;              //体表特征
    HabitualActionType HabitualMovement;      //习惯动作
    BehaviorType Behavior;                    //行为
    char BehaviorDescription[256];            //行为描述
    char Appendage[128];                      //附属物
    char AppendantDescription[256];           //附属物描述
    ColorType UmbrellaColor;                  //伞颜色
    ColorType RespiratorColor;                //口罩颜色
    HatStyleType CapStyle;                    //帽子款式
    ColorType CapColor;                       //帽子颜色
    GlassesStyleType GlassStyle;              //眼镜款式
    ColorType GlassColor;                     //眼镜颜色
    ColorType ScarfColor;                     //围巾颜色
    BagStyleType BagStyle;                    //包款式
    ColorType BagColor;                       //包颜色
    CoatStyleType CoatStyle;                  //上衣款式
    CoatLengthType CoatLength;                //上衣长度
    ColorType CoatColor;                      //上衣颜色
    PantsStyleType TrousersStyle;             //裤子款式
    ColorType TrousersColor;                  //裤子颜色
    PantsLengthType TrousersLen;              //裤子长度
    ShoesStyleType ShoesStyle;                //鞋子款式
    ColorType ShoesColor;                     //鞋子颜色
    queue_common_1400 SubImageList;           //图像列表
} Person;

typedef struct {
    ImageCntObjectIdType MotorVehicleID;          //车辆标识
    InfoType InfoKind;                            //信息分类
    BasicObjectIdType SourceID;                   //来源标识
    DeviceIDType TollgateID;                      //卡口编码
    DeviceIDType DeviceID;                        //设备编码
    char StorageUrl1[256];                        //近景照片
    char StorageUrl2[256];                        //车牌照片
    char StorageUrl3[256];                        //远景照片
    char StorageUrl4[256];                        //合成图
    char StorageUrl5[256];                        //缩略图
    int LeftTopX;                                 //车左上角右下角坐标
    int LeftTopY;                                 //左上
    int RightBtmX;                                //右下
    int RightBtmY;                                //右下
    dateTime MarkTime;                            //位置标记时间
    dateTime AppearTime;                          //车辆出现时间
    dateTime DisappearTime;                       //车辆消失时间
    int LaneNo;                                   //车道号
    Boolean HasPlate;                             //有无车牌
    PlateClassType PlateClass;                    //号牌种类
    ColorType PlateColor;                         //车牌颜色
    PlateNoType PlateNo;                          //车牌号
    PlateNoType PlateNoAttach;                    //挂车牌号
    char PlateDescribe[64];                       //车牌描述
    Boolean IsDecked;                             //是否套牌
    Boolean IsAltered;                            //是否涂改
    Boolean IsCovered;                            //是否遮挡
    SpeedType Speed;                              //行驶速度(单位千米每小时（km/h）)
    HDirectionType Direction;                     //行驶方向
    DrivingStatusCodeType DrivingStatusCode;      //行驶状态代码
    UsingPropertiesCodeType UsingPropertiesCode;  //车辆使用性质代码
    VehicleClassType VehicleClass;                //车辆类型
    VehicleBrandType VehicleBrand;                //车辆品牌
    VehicleModelType VehicleModel;                //车辆型号
    char VehicleStyles[32];                       //车辆年款
    VehicleLengthType VehicleLength;              //车辆长度
    VehicleWidthType VehicleWidth;                //车辆宽度
    VehicleHeightType VehicleHeight;              //车辆高度
    ColorType VehicleColor;                       //车身颜色
    VehicleColorDepthType VehicleColorDepth;      //颜色深浅
    char VehicleHood[64];                         //车前盖
    char VehicleTrunk[64];                        //车后盖
    char VehicleWheel[64];                        //车轮
    WheelPrintedPatternType WheelPrintedPattern;  //车轮印花纹
    char VehicleWindow[64];                       //车窗
    char VehicleRoof[64];                         //车顶
    char VehicleDoor[64];                         //车门
    char SideOfVehicle[64];                       //车侧
    char CarOfVehicle[64];                        //车厢
    char RearviewMirror[64];                      //后视镜
    char VehicleChassis[64];                      //底盘
    char VehicleShielding[64];                    //遮挡
    AutoFoilColorType FilmColor;                  //贴膜颜色
    Boolean IsModified;                           //改装标志
    DentInfoType HitMarkInfo;                     //撞痕信息
    char VehicleBodyDesc[128];                    //车身描述
    FrontThingType VehicleFrontItem;              //车前部物品
    char DescOfFrontItem[256];                    //车前部物品描述
    RearThingType VehicleRearItem;                //车后部物品
    char DescOfRearItem[256];                     //车后部物品描述
    int NumOfPassenger;                           //车内人数
    dateTime PassTime;                            //经过时刻
    char NameOfPassedRoad[64];                    //经过道路名称
    Boolean IsSuspicious;                         //是否可疑车
    int Sunvisor;                                 //遮阳板状态
    int SafetyBelt;                               //安全带状态
    int Calling;                                  //打电话状态
    char PlateReliability[4];                     //号牌识别可信度
    char PlateCharReliability[64];                //每位号牌号码可信度
    char BrandReliability[4];                     //品牌标志识别可信度
    queue_common_1400 SubImageList;               //图像列表
} MotorVehicle;

typedef struct {
    ImageCntObjectIdType NonMotorVehicleID;       //非机动车标识
    InfoType InfoKind;                            //信息分类
    BasicObjectIdType SourceID;                   //来源标识
    DeviceIDType DeviceID;                        //设备编码
    int LeftTopX;                                 //车左上角右下角坐标
    int LeftTopY;                                 //左上
    int RightBtmX;                                //右下
    int RightBtmY;                                //右下
    dateTime MarkTime;                            //位置标记时间
    dateTime AppearTime;                          //车辆出现时间
    dateTime DisappearTime;                       //车辆消失时间
    Boolean HasPlate;                             //有无车牌
    PlateClassType PlateClass;                    //号牌种类
    ColorType PlateColor;                         //车牌颜色
    PlateNoType PlateNo;                          //车牌号
    PlateNoType PlateNoAttach;                    //挂车牌号
    char PlateDescribe[64];                       //车牌描述
    Boolean IsDecked;                             //是否套牌
    Boolean IsAltered;                            //是否涂改
    Boolean IsCovered;                            //是否遮挡
    SpeedType Speed;                              //行驶速度(单位千米每小时（km/h）)
    DrivingStatusCodeType DrivingStatusCode;      //行驶状态代码
    UsingPropertiesCodeType UsingPropertiesCode;  //车辆使用性质代码
    char VehicleBrand[32];                        //车辆品牌
    char VehicleModel[64];                        //车辆款型
    VehicleLengthType VehicleLength;              //车辆长度
    VehicleWidthType VehicleWidth;                //车辆宽度
    VehicleHeightType VehicleHeight;              //车辆高度
    ColorType VehicleColor;                       //车身颜色
    char VehicleHood[64];                         //车前盖
    char VehicleTrunk[64];                        //车后盖
    char VehicleWheel[64];                        //车轮
    char WheelPrintedPattern[4];                  //车轮印花纹
    char VehicleWindow[64];                       //车窗
    char VehicleRoof[64];                         //车顶
    char VehicleDoor[64];                         //车门
    char SideOfVehicle[64];                       //车侧
    char CarOfVehicle[64];                        //车厢
    char RearviewMirror[64];                      //后视镜
    char VehicleChassis[64];                      //底盘
    char VehicleShielding[64];                    //遮挡
    int FilmColor;                                //贴膜颜色 0 深色 1 浅色 2 无
    int IsModified;                               //改装标志 0 未改装 1 改装
    queue_common_1400 SubImageList;               //图像列表
} NonMotorVehicle;

typedef struct {
    queue_common_1400 objList;
} ObjList;

typedef struct {
    int datatype;
    void* data;
} CallData;

typedef void (*picCallBack)(CallData* data, void* userArg);
void* gat1400_init(int threadnum, int port, const char* serverID, const char* localIp, const char* masterIp = "",
                      int useAuthuseAuthorization = 0, const char* username = "", const char* password = "");
int gat1400_uninit(void* handle);
int gat1400_start_capture(void* handle, const char* cameraVIID, picCallBack cb,
                        void* userArg, int mode = 0, const char* platformId = "",
                        const char* subType = "", const char* cameraType = "");
int gat1400_stop_capture(void* handle, const char* cameraVIID);
int gat1400_add_platform(void* handle, const char* platformVIID, const char* platformHost,
                       const int platformPort, const char* platformUsername = "", const char* platformPassword = "",
                       const char* platformType = "");
int gat1400_del_platform(void* handle, const char* platformVIID);
char* gat1400_query_device(void* handle, const char* platformVIID, const char* type, int pageIndex, int objNum);

#ifdef __cplusplus
}
#endif

#endif
