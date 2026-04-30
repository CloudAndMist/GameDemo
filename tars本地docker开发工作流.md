## 编译环境

### 进入容器命令

```bash
docker exec -it -w /workspace tars-cpp-compiler bash
```

### 服务端

#### web端申请部署服务

在运维管理→部署申请中[填写信息并发布](http://localhost:3000/tars.html#/operation/deploy)，OBJ节点为tars-node，即172.25.0.5

模版使用tars.default

#### 初始化新服务

```bash
cd /workspace/server
/usr/local/tars/cpp/script/cmake_tars_server.sh GameDemo ***Server(server) ***(obj)
```

#### 开发

[tars服务端开发方式](https://doc.tarsyun.com/#/base/tars-concept.md:~:text=6.-,%E6%9C%8D%E5%8A%A1%E7%AB%AF%E5%BC%80%E5%8F%91%E6%96%B9%E5%BC%8F,-%E4%BB%BB%E4%BD%95%20Tars%20%E6%9C%8D%E5%8A%A1)

- 进入源码目录

```
cd /workspace/server/***Server/src
```

- 修改.tars文件并重新生成***.h（如果需要）

```bash
/usr/local/tars/cpp/tools/tars2cpp ***.tars
```

- 在***Imp.cpp中实现业务逻辑
- 在***Server.cpp中注册新增servant

#### 编译

- 初次编译：创建build目录

```bash
cd /workspace/server/***Server
mkdir -p build
```

- 编译（[TOKEN获取](http://localhost:3000/auth.html#/token))

CMakeLists规范如引用其它tars服务或lib库见：[docs](https://doc.tarsyun.com/#/dev/tarscpp/tars-spec.md:~:text=Makefile%20%E6%A8%A1%E5%BC%8F%E4%BA%86-,1.%20cmake%20%E8%A7%84%E8%8C%83,-1.1.%20cmake%20%E4%BD%BF%E7%94%A8)

第三方包安装在/usr/local/tars/cpp/thirdparty目录下

```bash
cd /workspace/server/***Server/build
cmake .. -DTARS_WEB_HOST=http://tars-framework:3000 -DTARS_TOKEN=${TOKEN}
make -j4
```

- 打包上传

```bash
cd /workspace/server/***Server/build
make ***Server-tar
make ***Server-upload
```

- 接口修改后需要发布接口到[共享目录](https://doc.tarsyun.com/#/base/tars-concept.md:~:text=%E5%9D%97(nodejs%2Cphp)-,5.%20Tars%20%E6%96%87%E4%BB%B6%E7%9B%AE%E5%BD%95%E8%A7%84%E8%8C%83,-Tars%20%E6%96%87%E4%BB%B6%E6%98%AF) 

```bash
cd /workspace/server/***Server/build
make release
```

- 在web端选择版本并发布

#### 版本更新

- 如果修改了.tar文件

```bash
cd /workspace/server/***Server/src
/usr/local/tars/cpp/tools/tars2cpp ***.tars
```

- 再次编译、打包、上传、发布

```bash
cd /workspace/server/***Server/build
make clean
cmake ..
make
make ***Server-tar
make ***Server-upload
make ***Server-release
```

### 客户端

#### 创建

```bash
cd /workspace
mkdir -p client/***Client/src
cd ./src
```

#### 开发

[tars客户端开发方式]([tars服务端开发方式](https://doc.tarsyun.com/#/base/tars-concept.md:~:text=6.-,%E6%9C%8D%E5%8A%A1%E7%AB%AF%E5%BC%80%E5%8F%91%E6%96%B9%E5%BC%8F,-%E4%BB%BB%E4%BD%95%20Tars%20%E6%9C%8D%E5%8A%A1))

#### 编译

- 初始化

```bash
mkdir -p build
```

- 更新

```bash
cd /workspace/client/***Client/build
make clean
cmake ..
make
```

#### 运行

```bash
./TestHelloClient
```

