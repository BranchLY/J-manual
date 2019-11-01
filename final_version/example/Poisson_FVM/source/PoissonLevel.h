
#ifndef included_PoissonLevelIntegrator
#define included_PoissonLevelIntegrator

#include "TimeIntegratorLevelStrategy.h"
#include "InitializeIntegratorComponent.h"
#include "NumericalIntegratorComponent.h"
#include "DtIntegratorComponent.h"
#include "CopyIntegratorComponent.h"
#include "MemoryIntegratorComponent.h"
using namespace JAUMIN;

#include "Poisson.h"

/*该类从网格层时间积分算法策略类 algs::TimeIntegratorLevelStrategy 派生,
 实现Level层次上的求解流程.*/
class PoissonLevel : public algs::TimeIntegratorLevelStrategy<NDIM> {
public:
  /**
   * @brief 构造函数.
   * @param object_name     输入参数, 字符串, 表示对象名称.
   * @param patch_strategy  输入参数, 指针, Euler方程网格片积分算法.
   */
  PoissonLevel(const string& object_name, Poisson* patch_strategy);

  /**
   * @brief 析构函数.
   */
  virtual ~PoissonLevel();

  ///@name 重载基类algs::TimeIntegratorLevelStrategy<NDIM>的函数
  //@{
  /**
   * @brief 初始化: 创建所有计算需要的积分构件.
   * @param manager 输入参数, 指针, 指向积分构件管理器.
   */
  void initializeLevelIntegrator(
      tbox::Pointer<algs::IntegratorComponentManager<NDIM> > manager);

  /**
   * @brief 初始化指定网格层的数据片.
   *
   * 具体地，该函数完成以下操作：
   * - 若输入参数 initial_time 为真，则根据初始条件，
   *   为当前网格层上的所有数据片 <uval,current> 赋初值;
   * - 若输入参数 initial_time 为假（此时刚完成负载调整），
   *   则将数据片 <uval,current> 的值从旧网格层复制到新网格层.
   *
   * @param level 输入参数, 指针, 指向待初始化网格层.
   * @param init_data_time 输入参数, 双精度浮点型, 表示初始化的时刻.
   * @param initial_time 输入参数, 逻辑型, 真值表示当前时刻为计算的初始时刻.
   *
   * @note
   * 该函数调用了1个初值构件对象，该对象又进一步自动调用函数
   * Poisson::initializePatchData(), 逐个网格片地完成初始时刻的数据初始化.
   */
  void initializeLevelData(
      const tbox::Pointer<hier::BasePatchLevel<NDIM> > level,
      const double init_data_time, const bool initial_time = true);

  /**
   * @brief 返回指定网格层的时间步长.
   *
   * @param level 输入参数, 指针, 指向网格层.
   * @param dt_time 输入参数, 双精度浮点型, 表示计算时间步长的当前时刻.
   * @param initial_time 输入参数, 逻辑型, 真值表示当前时刻为计算的初始时刻.
   * @param flag_last_dt 输入参数, 整型, 表示上个时间步积分返回的状态.
   * @param last_dt 输入参数, 双精度浮点型, 表示上个时间步长.
   * @return 双精度浮点型, 表示网格层的时间步长.
   *
   * @note 该函数调用了1个时间步长构件对象，步长构件对象又进一步自动调用网格片上
   *       的求步长函数, 网格片上的步长函数再调用用户写的步长函数.
   *
   * Poisson::getPatchDt(), 逐个网格片地计算稳定时间步长.
   */
  double getLevelDt(const tbox::Pointer<hier::BasePatchLevel<NDIM> > level,
                    const double dt_time, const bool initial_time,
                    const int flag_last_dt, const double last_dt);

  /**
   * @brief 网格层向前积分一个时间步.
   *
   * @param level 输入参数, 指针, 指向待积分的网格层.
   * @param current_time 输入参数, 双精度浮点型, 表示时间步的起始时刻.
   * @param predict_dt 输入参数, 双精度浮点型, 表示为该时间步预测的时间步长.
   * @param max_dt 输入参数, 双精度浮点型, 表示时间步允许的最大时间步长.
   * @param min_dt 输入参数, 双精度浮点型, 表示时间步允许的最小时间步长.
   * @param first_step 输入参数, 逻辑型, 真值当前为重构后或时间步序列的第1步.
   * @param step_number, 输入参数, 整型, 表示积分步数.
   * @param actual_dt 输出参数, 双精度浮点型, 表示时间步实际采用的时间步长.
   * @return 整型, 表示该时间步积分的状态.
   *
   * @note
   * 该函数调用了2个数值构件对象，
   * 分别完成新时刻的通量 f 和守恒量 u 的计算.
   * 相关的网格片计算函数见 Poisson::computeOnPatch().
   */
  int advanceLevel(const tbox::Pointer<hier::BasePatchLevel<NDIM> > level,
                   const double current_time, const double predict_dt,
                   const double max_dt, const double min_dt,
                   const bool first_step, const int step_number,
                   double& actual_dt);

  /**
   * @brief 更新网格层的状态到新的时刻.
   *
   * @param level 输入参数, 指针, 指向网格层.
   * @param new_time 输入参数, 双精度浮点型, 表示新的时刻.
   * @param deallocate_data 输入参数, 逻辑型, 真值表示接收数值解后,
   * 释放新值数据片的内存空间.
   *
   * @note
   * 该函数调用复制构件, 将数据从新值复制到当前值上下文的数据片.
   */
  void acceptTimeDependentSolution(
      const tbox::Pointer<hier::BasePatchLevel<NDIM> > level,
      const double new_time, const bool deallocate_data);

  //@}

private:

  /*!@brief Poisson方程的Patch类. */
  Poisson* d_patch_strategy;

  /*!@brief 初值构件: 管理current时间步数据片的内存和为物理量赋初值 */
  tbox::Pointer<algs::InitializeIntegratorComponent<NDIM> > d_init_set_value;
  /*!@brief 内存构件: 管理new时间步数据片的内存开辟及释放. */
  tbox::Pointer<algs::MemoryIntegratorComponent<NDIM> > d_alloc_new_data;
  /*!@brief 内存构件: 管理scratch时间步数据片的内存开辟及释放. */
  tbox::Pointer<algs::MemoryIntegratorComponent<NDIM> > d_alloc_scratch_data;
  /*!@brief 步长构件: 计算时间步长 */
  tbox::Pointer<algs::DtIntegratorComponent<NDIM> > d_dt_component;
  /*!@brief 数值构件: 计算新时间步物理量u */
  tbox::Pointer<algs::NumericalIntegratorComponent<NDIM> > d_value_computing;
  /*!@brief 数值构件: 计算通量 f  */
  tbox::Pointer<algs::NumericalIntegratorComponent<NDIM> > d_compute_flux;

  /*!@brief 复制构件：更新物理量 */
  tbox::Pointer<algs::CopyIntegratorComponent<NDIM> > d_copy_solution;
};

#endif
