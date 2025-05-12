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

更新命令：

```
在代码仓库对应目录下执行以下命令
1. selection_fwk仓库：
git clone https://gitee.com/sinall/selection_fwk.git 
2. 其余仓库
$ cd foundation/systemabilitymgr/samgr（修改为对应仓库路径）
$ git remote add sinall https://gitee.com/sinall/systemabilitymgr_samgr.git（修改为对应仓库的网址）
$ git fetch sinall selection_fwk
$ git checkout -b selection_fwk sinall/selection_fwk
```
