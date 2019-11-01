#include "PoissonLevel.h"

/*************************************************************************
 *                             构造函数                                   *
 *************************************************************************/
PoissonLevel::PoissonLevel(const string& object_name,
                                           Poisson* patch_strategy) {
#ifdef DEBUG_CHECK_ASSERTIONS
  assert(!object_name.empty());
  assert(patch_strategy != NULL);
#endif

  d_patch_strategy = patch_strategy;
}


/************************************************************************
 *                             析构函数                                  *
 ************************************************************************/
PoissonLevel::~PoissonLevel() {}



/*************************************************************************
 *                      创建计算需要的所有构件                               *
 *************************************************************************/
 /* 该函数创建了1个初值构件, 2个内存构件，1个时间步长构件，3个数值构件,
 *这些构件所操作的数据片, 由Patch层次的initializeComponent() 指定.*/

void PoissonLevel::initializeLevelIntegrator(
    tbox::Pointer<algs::IntegratorComponentManager<NDIM> > manager) {
  //初值构件: 管理current时间步数据片的内存和为物理量赋初值
  d_init_set_value = new algs::InitializeIntegratorComponent<NDIM>(
      "INIT_SET_VALUE", d_patch_strategy, manager);
  //内存构件: 管理new时间步数据片的内存开辟及释放.
  d_alloc_new_data = new algs::MemoryIntegratorComponent<NDIM>(
      "ALLOC_NEW_DATA", d_patch_strategy, manager);
  //内存构件: 管理scratch时间步数据片的内存开辟及释放.
  d_alloc_scratch_data = new algs::MemoryIntegratorComponent<NDIM>(
      "ALLOC_SCRATCH_DATA", d_patch_strategy, manager);

  //步长构件: 管理时间步长求解.
  d_dt_component = new algs::DtIntegratorComponent<NDIM>(
      "Dt", d_patch_strategy, manager);
  //数值构件: 计算新时间步物理量u
  d_value_computing = new algs::NumericalIntegratorComponent<NDIM>(
      "COMPUTING", d_patch_strategy, manager);
  //数值构件: 计算通量f
  d_compute_flux = new algs::NumericalIntegratorComponent<NDIM>(
      "COMPUTE_FLUX", d_patch_strategy, manager);
  //复制构件：新时间步上更新物理量
  d_copy_solution = new algs::CopyIntegratorComponent<NDIM>(
      "COPY_SOLUTION", d_patch_strategy, manager);
}

/*************************************************************************
 *                              初始化                                    *
 ************************************************************************/
/* 函数调用了初值构件（d_init_set_value)
 * 该构件又进一步自动调用 Patch层次的initializePatchData()，完成数据片的初始化.*/
void PoissonLevel::initializeLevelData(
    const tbox::Pointer<hier::BasePatchLevel<NDIM> > level,
    const double init_data_time, const bool initial_time) {
  d_init_set_value->initializeLevelData(level, init_data_time, initial_time);
  
}

/*************************************************************************
 *                            计算时间步长                                *
 ************************************************************************/
/*函数调用了步长构件(d_step_size)
 * 该构件对象又进一步调用 Patch层次的 getPatchDt(),逐个网格片地计算时间步长.*/
double PoissonLevel::getLevelDt(
    const tbox::Pointer<hier::BasePatchLevel<NDIM> > level,
    const double dt_time, const bool initial_time, const int flag_last_dt,
    const double last_dt) {
#ifdef DEBUG_CHECK_ASSERTIONS
  assert(!level.isNull());
#endif
  return(d_dt_component->getLevelDt(level, dt_time, initial_time,
                                       flag_last_dt, last_dt, false));
}

/*************************************************************************
 *                          向前积分一个时间步                              *
 ************************************************************************/
/*函数调用了2个数值构件对象的computing()函数，分别完成新时刻的通量f和守恒量u的计算.*/
int PoissonLevel::advanceLevel(
    const tbox::Pointer<hier::BasePatchLevel<NDIM> > level,
    const double current_time, const double predict_dt, const double max_dt,
    const double min_dt, const bool first_step, const int step_number,
    double& actual_dt) {
#ifdef DEBUG_CHECK_ASSERTIONS
  assert(!level.isNull());
#endif

  if (first_step) {
    //如果是第一个时间步，开辟new时间步数据片内存
    d_alloc_new_data->allocatePatchData(level, current_time + predict_dt); 
    
  }
  
  //开辟scratch时间步数据片内存
  d_alloc_scratch_data->allocatePatchData(level, current_time + predict_dt);
  // 计算通量f
  d_compute_flux->computing(level, current_time, predict_dt, false);
  
  // 计算守衡量u
  d_value_computing->computing(level, current_time, predict_dt, false);

  // 设置新值数据片的时间戳.
  d_alloc_new_data->setTime(level, current_time + predict_dt);
  // 释放scratch时间步数据片内存
  d_alloc_scratch_data->deallocatePatchData(level);
  //必须写的！！！将时间步长返回
  actual_dt = predict_dt;

  return (0);
}

/*************************************************************************
 *                            接收数值解                                  *
 ************************************************************************/
/*函数调用复制构件，将current时间步数据片的值复制到scratch时间步数据片中.*/
void PoissonLevel::acceptTimeDependentSolution(
    const tbox::Pointer<hier::BasePatchLevel<NDIM> > level,
    const double new_time, const bool last_step) {
    //复制数据片，源数据片与目标数据片由Patch层次的initializeComponent() 指定
    d_copy_solution->copyPatchData(level,new_time);
    //如果是最后一步，释放new时间步数据片内存
    if (last_step) {
        d_alloc_new_data->deallocatePatchData(level);
  }
}
