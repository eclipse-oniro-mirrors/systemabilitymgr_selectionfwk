# selectionfwk

## 介绍
从API version 20开始，新增划词服务模块，该模块具有跨应用文本处理及系统管理划词应用的能力。

该模块主要用于文本翻译、内容摘要、术语解释等场景。例如用户选中外文新闻中的单词时，可显示翻译结果；在阅读类应用中，可通过选中内容快速生成内容摘要；在学习类应用中，可识别用户选中的专业术语，提供即时解释卡片等。

### 框架原理

划词服务主要依托多模和剪贴板等领域，对外提供全局划词功能。该服务的核心业务流程主要分为以下几个步骤：
- 步骤一：划词服务可以根据系统设置中的开关按需启停。在启动时，会拉起用户选择的划词应用（未选择时，默认拉起最早安装的划词应用）。对应下图序号①②。
- 步骤二：在运行时，会监听多模事件以识别用户划词操作。在识别到用户想要触发划词后，会向剪贴板传递划词标记并注册回调函数。同时，会通过多模向被划词应用注入模拟的CTRL+C操作。对应下图序号③④⑤⑥。
- 步骤三：被划词应用在收到CTRL+C后，会触发复制操作，将当前用户选中的内容写入剪贴板。剪贴板在收到数据后，会回传文本内容给划词服务。最终，划词服务将内容传递给划词应用，由划词应用进行相应业务逻辑的处理，弹出划词窗口。对应下图序号⑦⑧⑨。

![划词服务框架原理图](figures/selection-service-schematic.png)

### 仓路径
/foundation/systemabilitymgr/selectionfwk

### 接口说明
当前划词服务提供的接口均为系统接口，暂未对三方划词应用开放。划词服务接口详细说明请参考[划词扩展能力](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-basic-services-kit/js-apis-selectionInput-selectionExtensionAbility-sys.md)和[划词管理](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-basic-services-kit/js-apis-selectionInput-selectionManager-sys.md)。

### 约束限制
- 支持外接键盘和鼠标的[PC/2in1](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/quick-start/module-configuration-file.md#devicetypes标签)设备。

- 支持获取文本类型的划词内容，最大长度限制为6000个字节。

- 支持在扩展屏上使用，不支持跨设备使用。

- 当前接口为系统接口，计划于API version 22开放为公共接口。

更多介绍见[划词服务应用开发文档](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/basic-services/selectionInput/Readme-CN.md)。


## 目录

```
/foundation/systemabilitymgr
├── selectionfwk
│   ├── common                                      # 公共代码
│   ├── etc                                         # 组件包含的进程的配置文件
│   ├── figures                                     # 框架原理图
│   ├── frameworks                                  # 接口实现
│   │   ├── js/napi                                 # 划词框架napi接口
│   │   └── native                                  # native接口
│   ├── hiappevent_agent                            # hiappevent平台打点
│   ├── interfaces                                  # idl接口定义文件
│   ├── sa_profile                                  # sa定义
│   ├── services                                    # 划词框架服务
│   ├── sysevent                                    # hisysevent平台打点
│   ├── test                                        # 接口测试目录
│   │   ├── fuzztest                                # fuzz测试
│   │   └── unitest                                 # 接口的单元测试
│   ├── utils                                       # 核心服务工具代码目录
```

## 编译步骤

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

## 测试步骤

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