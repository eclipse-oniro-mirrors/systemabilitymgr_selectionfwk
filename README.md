## 划词服务代码环境搭建

### 代码结构

```
│   ├── base
│   │   ├── startup
│   │   │   └── init            => 代码提交仓: https://gitee.com/sinall/startup_init.git, selection_fwk分支
│   │   └── security
│   │       └── selinux_adapter => 代码提交仓: https://gitee.com/sinall/security_selinux_adapter.git, selection_fwk分支
│   ├── foundation
│   │   └── systemabilitymgr
│   │       ├── samgr           => 代码提交仓: https://gitee.com/sinall/systemabilitymgr_samgr.git, selection_fwk分支
│   │       └── selection_fwk   => 代码提交仓: https://gitee.com/sinall/selection_fwk.git, master分支
│   ├── productdefine
│   │   	└── common          => 代码提交仓: https://gitee.com/sinall/productdefine_common.git, selection_fwk分支
```

### 更新代码命令

- 初次环境搭建，在代码仓库对应目录下执行以下命令

```
1. selection_fwk仓库：
cd foundation/systemabilitymgr
git clone https://gitee.com/sinall/selection_fwk.git 
2. 其余仓库
$ cd foundation/systemabilitymgr/samgr（修改为对应仓库路径）
$ git remote add sinall https://gitee.com/sinall/systemabilitymgr_samgr.git（修改为对应仓库的网址）
$ git fetch sinall selection_fwk
$ git checkout -b selection_fwk sinall/selection_fwk
```

- 后续更新代码

```
cd foundation/systemabilitymgr（修改为对应的仓库路径）
git pull --reb sinall master（修改为对应的分支名）
```

### 编译命令

- 全量编译

```
修改build.gn文件后编译命令
$ ./build.sh --product-name rk3568 --ccache
未修改build.gn文件编译命令
$ ./build.sh --product-name rk3568 --ccache --fast-rebuild
```

- 单独编译

```
$ ./build.sh --product-name rk3568 --ccache --build-target selectionfwk
```

### 测试命令

1. 测试启动服务

```
# ps -ef | grep selection_service
未启动服务
# param set sys.selection.switch.username on
# ps -e未启动服务f | grep selection_service
服务已启动
```

2. 测试关闭服务 （系统参数关闭会延时，等待约20-30s）

```
# ps -ef | grep selection_service
服务已启动
# param set sys.selection.switch.username off
# ps -ef | grep selection_service
服务已退出
```

3. 获取日志命令

```
# dmesg | grep selection_service
# hilog -b D
# hilog | grep 8500
```
