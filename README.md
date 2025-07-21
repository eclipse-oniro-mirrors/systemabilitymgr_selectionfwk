# selectionfwk

#### 介绍
划词服务,通过剪贴板获取划词内存，提供划词配置同步接口，划词服务SA监听多模键鼠事件获取鼠标和触控板操作窗口和应用信息，DFX增强。

#### 仓路径
/foundation/systemabilitymgr/selectionfwk

## 目录

```
/foundation/systemabilitymgr
├── selectionfwk
│   ├── common                                      # 公共代码
│   ├── etc                                         # 组件包含的进程的配置文件
│   ├── frameworks                                  # 接口实现
│   │   └── js/napi                                 # 划词框架napi接口
│   │   └── native                                  # native接口
│   ├── sa_profile                                  # sa定义
│   ├── services                                    # 划词框架服务
│   ├── test                                        # 接口测试目录
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
# param set sys.selection.trigger ""
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