#ifndef LEVELCOMPUTE_H
#define LEVELCOMPUTE_H

#include <TimeIntegratorLevelStrategy.h> // 网格层时间积分算法策略基类
// 解法器管理类 和 线性解法器基类
#include "LinearSolverManager.h"
#include "BaseLinearSolver.h"
// 通过以下积分构件组织流程（IntegratorComponent基类的派生类）
#include <InitializeIntegratorComponent.h> // 初值构件
#include <DtIntegratorComponent.h>         // 步长构件
#include <NumericalIntegratorComponent.h>  // 数值构件
#include <MemoryIntegratorComponent.h>     // 内存构件
// #include <CopyIntegratorComponent.h>        // 复制构件
// #include <ReductionIntegratorComponent.h>   // 规约构件
// #include <SynchronizeIntegratorComponent.h> // 同步构件
// ...

using namespace JAUMIN;

#include "PatchCompute.h"
/**
 * @brief 用户定义的网格层积分算法类，继承自抽象基类TimeIntegratorLevelStrategy
 * 必须实现其基类的纯虚函数
 *
 * 该类主要初始化各种积分构件，并使用这些积分构件组织计算流程
 */
class LevelCompute : public algs::TimeIntegratorLevelStrategy<NDIM>
{

public:
    // =======构造函数：
    LevelCompute(
        const string &object_name,               // 对象名称
        PatchCompute *patch_strategy,            // 用户定义的时间片积分算法
        tbox::Pointer<tbox::Database> input_db); // 解法器数据库

    // =======析构函数：
    virtual ~LevelCompute(); // 虚析构函数，确保delete一个指向派生类的基类指针可以执行正确的析构函数版本

    // =======实现基类抽象接口函数
    // 1. 初始化积分算法：创建所有计算需要的积分构件
    void initializeLevelIntegrator(
        tbox::Pointer<algs::IntegratorComponentManager<NDIM> > manager); // 管理积分构件的 注册、初始化、注销等

    // 2. 组织调用积分构件实现具体的数值计算流程：
    // *****(1) 初始化指定网格层的数据片：借助初值构件类algs::InitializeIntegratorComponent对象
    //          （该初值构件对象又进一步自动调用标准构件网格片策略基类(用户派生自定义)中的接口函数initializePatchData()）
    //          可以 辅助实现 网格层上所有网格片上数据片的初始化
    void initializeLevelData(
        const tbox::Pointer<hier::BasePatchLevel<NDIM> > level, // 指向待初始化网格层
        const double init_data_time,                           // 初始化时刻
        const bool initial_time = true);                       // 真值表示当前时刻为计算的初始时刻

    // *****(2) 获取指定网格层的时间步长：借助步长构件类algs::DtIntegratorComponent对象
    //          （该步长构件对象又进一步自动调用标准构件网格片策略基类(用户派生自定义)中的接口函数getPatchDt()）
    //          可以 辅助实现 计算对所有网格片稳定的时间步长
    double getLevelDt(                                         // 返回值为当前时间步的预测步长
        const tbox::Pointer<hier::BasePatchLevel<NDIM> > level, // 待获取时间步长的网格层
        const double dt_time,                                  // 计算时间步长的当前时刻
        const bool initial_time,                               // 真值表示当前时刻为计算的初始时刻
        const int flag_last_dt,                                // 上个时间步积分返回的状态
        const double last_dt);                                 // 上个时间步长

    // *****(3) 在指定网格层上积分一个时间步：借助数值构件类 algs::NumericalIntegratorComponent对象
    //          （该数值构件对象又进一步自动调用标准构件网格片策略基类(用户派生自定义)中的接口函数setPhysicalBoundaryConditions()和computeOnPatch()）
    //          可以 辅助实现 将数值解推进一个时间步时刻（一般也会借助内存构件管理中间量数据片以及其他可能的构件）
    int advanceLevel(                                          // 返回值的含义由用户负责解释，表示该时间步积分的状态
        const tbox::Pointer<hier::BasePatchLevel<NDIM> > level, // 待积分的网格层（注：要求断言检查指针level非空）
        const double current_time,                             // 时间步的起始时刻
        const double predict_dt,                               // 该时间步预测的时间步长
        const double max_dt,                                   // 时间步允许的最大时间步长
        const double min_dt,                                   // 时间步允许的最小时间步长
        const bool first_step,                                 // 表示当前为重构后or重启动后or时间步序列的第1个时间步（判断是否为计算数值解的辅助数据片调度内存）
        const int hierarchy_step_number,                       // 表示网格片层次结构的积分步数（也就是最粗网格层的积分步数）
        double &actual_dt);                                    // 时间步实际采用的时间步长

private:
    // 对象名称和有限元网格片积分算法对象
    string d_object_name;
    PatchCompute *d_patch_strategy;

    // 解法器管理器 和 解法器: 求解矩阵系统
    tbox::Pointer<solv::LinearSolverManager<NDIM> > d_solver_manager;
    tbox::Pointer<solv::BaseLinearSolver<NDIM> > d_solver;
    // 解法器数据库.
    tbox::Pointer<tbox::Database> d_solver_db; // d_solver->solve() 的参数

    // 指向辅助构件的指针数据成员：
    // 1. 初值构件（管理数据片的内存以及初始化）
    tbox::Pointer<algs::InitializeIntegratorComponent<NDIM> > d_init;

    // 2. 步长构件
    tbox::Pointer<algs::DtIntegratorComponent<NDIM> > d_step;

    // 3. 数值构件
    // 矩阵构件: 不注册数据片填充影像区，仅调用数据片策略类中的computeOnpatch进行矩阵组装
    tbox::Pointer<algs::NumericalIntegratorComponent<NDIM> > d_cmp_equation;

    // 本例中将解向量中的数据赋值到plot绘图数据（还可以进行场分析等后处理操作）
    tbox::Pointer<algs::NumericalIntegratorComponent<NDIM> > d_cmp_post;

    // 4. 内存构件
    // 管理有限元方程求解中的向量数据片内存分配
    tbox::Pointer<algs::MemoryIntegratorComponent<NDIM> > d_alloc;
};

#endif
