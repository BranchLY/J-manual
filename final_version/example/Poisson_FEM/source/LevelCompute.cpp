#include <assert.h>
#include "LevelCompute.h"

/***********************************************************************************
                                                                  注意： 前缀为“d_”的变量为成员变量
 ***********************************************************************************/

// 构造函数：
LevelCompute::LevelCompute(
    const string &object_name,               // 对象名称
    PatchCompute *patch_strategy,            // 用户定义的网格片算法对象
    tbox::Pointer<tbox::Database> solver_db) // 数据库，此处传入的是解法器数据库
{
#ifdef DEBUG_CHECK_ASSERTIONS // 参数检查
    assert(!object_name.empty());
    assert(patch_strategy != NULL);
#endif
    // 成员函数：网格片算法对象
    d_patch_strategy = patch_strategy;
    d_solver_db=solver_db;
    //解法器（第6章）
    d_solver_manager = solv::LinearSolverManager<NDIM>::getManager();                     //获取线性解法器管理器
    d_solver = d_solver_manager->lookupLinearSolver(solver_db->getString("solver_name")); //获取解法器
}

// 析构函数：
LevelCompute::~LevelCompute() {}

//创建构件
void LevelCompute::initializeLevelIntegrator(
    tbox::Pointer<algs::IntegratorComponentManager<NDIM> > manager) // 管理积分构件的 注册、初始化、注销等
{
     /******************************创建构件（5.2节）*************************************/
    // 创建初值构件，d_init管理名为"INIT_VALUES"的初值构件
    d_init = new algs::InitializeIntegratorComponent<NDIM>("INIT_VALUES", d_patch_strategy, manager);
    // 创建步长构件，d_step管理名为"STEP_SIZE"的步长构件
    d_step = new algs::DtIntegratorComponent<NDIM>("STEP_SIZE", d_patch_strategy, manager);
    // 创建数值构件
    d_cmp_equation = new algs::NumericalIntegratorComponent<NDIM>("CMP_EQUATION", d_patch_strategy, manager);
    d_cmp_post = new algs::NumericalIntegratorComponent<NDIM>("CMP_POST", d_patch_strategy, manager);
    // 创建内存构件
    d_alloc = new algs::MemoryIntegratorComponent<NDIM>("ALLOC", d_patch_strategy, manager);
}

// 网格层上数据片的初始化
void LevelCompute::initializeLevelData(
    const tbox::Pointer<hier::BasePatchLevel<NDIM> > level, // 指向待初始化网格层
    const double init_data_time,                           // 初始化时刻
    const bool initial_time)                               // 真值表示当前时刻为计算的初始时刻
{
    //初值构件调用该接口，为注册到初值构件的数据片分配内存
    d_init->initializeLevelData(level, init_data_time, initial_time);
}

// 网格层上时间步长设置
double LevelCompute::getLevelDt(                           // 返回值为当前时间步的预测步长
    const tbox::Pointer<hier::BasePatchLevel<NDIM> > level, // 待获取时间步长的网格层
    const double dt_time,                                  // 计算时间步长的当前时刻
    const bool initial_time,                               // 真值表示当前时刻为计算的初始时刻
    const int flag_last_dt,                                // 上个时间步积分返回的状态
    const double last_dt)                                  // 上个时间步长
{
    //步长构件调用该接口
    return d_step->getLevelDt(level, dt_time, initial_time, flag_last_dt, last_dt);
}

// 搭建网格片的数值计算流程
int LevelCompute::advanceLevel(                            // 返回值的含义由用户负责解释，表示该时间步积分的状态
    const tbox::Pointer<hier::BasePatchLevel<NDIM> > level, // 待积分的网格层（注：要求断言检查指针level非空）
    const double current_time,                             // 时间步的起始时刻
    const double predict_dt,                               // 该时间步预测的时间步长
    const double max_dt,                                   // 时间步允许的最大时间步长
    const double min_dt,                                   // 时间步允许的最小时间步长
    const bool first_step,                                 // 表示当前为重构后or重启动后or时间步序列的第1个时间步
                                                           // 第一次进入本函数为True，以后都为False（判断是否为计算数值解的中间数据片调度内存）
    const int hierarchy_step_number,                       // 表示网格片层次结构的积分步数（也就是最粗网格层的积分步数）
    double &actual_dt)                                     // 时间步实际采用的时间步长
{

    //内存构件调用该接口，为注册在内存构件的数据片分配内存（5.4.2节）
    d_alloc->allocatePatchData(level, current_time);

    //各类型数值构件调用接口执行计算
    // 组装左端矩阵，右端向向量，解向量
    d_cmp_equation->computing(level, current_time, predict_dt);
    // 自定义函数获取数据片ID
    int mat_id = d_patch_strategy->getMatrixID();
    int vec_id = d_patch_strategy->getRHSID();
    int sol_id = d_patch_strategy->getSolutionID();
    // 利用解法器求解方程组（6.2节）
    d_solver->setMatrix(mat_id);
    d_solver->setRHS(vec_id);
    d_solver->solve(first_step, sol_id, level, d_solver_db);
    //  后处理计算
    d_cmp_post->computing(level, current_time, predict_dt);
    actual_dt = predict_dt;//将预测时间步长传给真实时间步，必须要
    // 释放指定网格层中内存构件分配的临时数据片
    d_alloc->deallocatePatchData(level);

    return 0;
}
