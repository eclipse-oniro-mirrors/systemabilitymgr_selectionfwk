# selectionfwk

#### 介绍
从API version 20开始，新增支持划词服务，提供跨应用文本处理及系统管理划词应用的能力。

主要用于文本翻译、内容摘要、术语解释等场景，例如用户长按外文新闻中的单词时，自动触发翻译服务并悬浮显示结果；在阅读应用中划选长段落，通过划词服务快速生成内容摘要；教育类应用识别用户选中的专业术语，提供即时解释卡片等。

#### 仓路径
/foundation/systemabilitymgr/selectionfwk

#### 接口说明
当前划词服务提供的接口均为系统接口，暂未对三方划词应用开放。划词服务接口详细说明请参考[划词扩展能力](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-basic-services-kit/js-apis-selectionInput-selectionExtensionAbility-sys.md)和[划词管理](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-basic-services-kit/js-apis-selectionInput-selectionManager-sys.md)。

#### 约束限制
- 支持外接键盘和鼠标的[2in1](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/quick-start/module-configuration-file.md#devicetypes标签)设备。

- 支持获取文本类型的划词内容，最大长度限制为6000个字节。

更多介绍见[划词服务应用开发文档](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/basic-services/selectionInput/Readme-CN.md)。


## 目录

```
/foundation/systemabilitymgr
├── selectionfwk
│   ├── common                                      # 公共代码
│   ├── etc                                         # 组件包含的进程的配置文件
│   ├── frameworks                                  # 接口实现
│   │   ├── js/napi                                 # 划词框架napi接口
│   │   └── native                                  # native接口
│   ├── hiappevent_agent                            # hiappevent平台打点
│   ├── interfaces                                  # idl接口定义文件
│   ├── sa_profile                                  # sa定义
│   ├── services                                    # 划词框架服务
│   ├── test                                        # 接口测试目录
│   │   ├── fuzztest                                # fuzz测试
│   │   └── unitest                                 # 接口的单元测试
│   ├── utils                                       # 核心服务工具代码目录
```

### 编译步骤

- 全量编译

修改build.gn文件后编译命令
```
$ ./build.sh --product-name rk3568 --ccache
```
未修改build.gn文件编译命令
```
$ ./build.sh --product-name rk3568 --ccache --fast-rebuild
```

- 单独编译

```
$ ./build.sh --product-name rk3568 --ccache --build-target selectionfwk
```

### 测试步骤

1. 测试方法

查看服务进程
```
# ps -ef | grep selection
```
启动划词服务
```
# param set sys.selection.switch on
```
关闭划词服务
```
# param set sys.selection.switch off
```
切换划词应用
```
# param set sys.selection.app com.selection.selectionapplication/SelectionExtensionAbility
```
设置划词触发方式
```
# param set sys.selection.trigger "immediate"
```

2. 获取日志命令

打开debug日志
```
# hilog -b D
```
过滤日志
```
# hilog -T SELECTION_SERVICE
```

## 参与贡献

1.  Fork 本仓库
2.  提交代码
3.  新建 Pull Request
4.  commit完成即可