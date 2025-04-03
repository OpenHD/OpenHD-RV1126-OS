## RK3562/RK3566/RK3568/RK3588/RV1103/RV1106 RKNN SDK 1.6.0

### 主要修改

- 支持OPSET 12~19 的ONNX模型
- 支持自定义算子（含CPU及GPU）
- 优化算子支持如动态权重的卷积、Layernorm、RoiAlign、Softmax、ReduceL2、Gelu、GLU等
- 新增对python3.7/3.9/3.11支持
- 添加rknn_convert的功能
- 优化transformer支持
- 优化 MatMul API，如增大K限制长度，RK3588新增int4 * int4 -> int16 计算支持等
- 减少RV1106 rknn_init初始化耗时，降低内存消耗等
- RV1106 新增部分算子的int16支持
- 修复RV1106平台卷积在部分情况下随机出错问题
- 优化用户手册
- 重构rknn model zoo，新增检测、分割、OCR、车牌识别等多种模型支持

### 版本号查询

- librknnrt runtime版本：1.6.0（strings librknnrt.so | grep version | grep lib）
  
- rknpu driver版本：0.9.3（dmesg | grep rknpu）

### 其他说明

- rknn-toolkit适用RV1109/RV1126/RK1808/RK3399Pro，rknn-toolkit2适用RK3562/RK3566/RK3568/RK3588/RV1103/RV1106
- rknn-toolkit2与rknn-toolkit API接口基本保持一致，用户不需要太多修改（rknn.config()部分参数有删减）
- rknpu2需要与rknn-toolkit2同步升级到1.6.0的版本。之前使用rknn toolkit2 1.5.0版本生成的rknn模型可以不重新生成
- rknn api里面部分demo依赖MPI MMZ/RGA，使用时需要和系统中相应的库匹配  

## RK356X/RK3588/RV1103/RV1106 RKNN SDK 1.5.2

### 主要修改

#### RKNN-Toolkit2 1.5.2

- 优化transformer支持
- 优化动态shape支持
- 优化Matmul 独立API支持
- 优化rknn.config的target_platform默认参数
- 优化GRU/LSTM/transpose/reshape/softmax/maxpool等算子支持
- 优化大分辨率模型支持
- 优化LLM支持
- 优化多batch支持
- rknn runtime增加GPU后端框架支持，并实现部分算子，如matmul等
- 优化rknn_init初始化耗时
- 降低rknn_init初始化内存占用

###  版本号查询

- librknnrt runtime版本：1.5.2（strings librknnrt.so | grep version | grep lib）
- rknpu driver版本：0.9.2（dmesg | grep rknpu）

### 其他说明

- rknn-toolkit适用RV1109/RV1126/RK1808/RK3399Pro，rknn-toolkit2适用RK3562/RK3566/RK3568/RK3588/RV1103/RV1106
- rknn-toolkit2与rknn-toolkit API接口基本保持一致，用户不需要太多修改（rknn.config()部分参数有删减）
- rknpu2需要与rknn-toolkit2同步升级到1.5.2的版本。之前客户使用rknn toolkit2 1.5.0版本生成的rknn模型可以不重新生成
- rknn api里面部分demo依赖MPI MMZ/RGA，使用时，需要和系统中相应的库匹配   

