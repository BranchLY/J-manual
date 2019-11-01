#ifndef included_Poisson
#define included_Poisson
#include "DOFInfo.h"
#include "JaVisDataWriter.h"
#include "Database.h"
#include "StandardComponentPatchStrategy.h"
#include "CellVariable.h"


using namespace std;
using namespace JAUMIN;

/**
 * @brief 该类从标准构件网格片策略类 algs::StandardComponentPatchStrategy 派生,
 * 实现求解Poisson方程的数值计算子程序.
 *
 */
class Poisson:public algs::StandardComponentPatchStrategy<NDIM> {
public:
  /*! @brief 构造函数.
   * @param object_name 输入参数, 字符串, 表示对象名称.
   * @param input_db    输入参数, 指针, 指向输入数据库.
   *
   * @note
   * 该函数主要完成以下操作:
   *  -# 初始化内部数据成员;
   *  -# 定义变量和数据片.
   */
  Poisson(const string& object_name, tbox::Pointer<tbox::Database> input_db);

  /*!
   * @brief 析构函数.
   */
  virtual ~Poisson();

  /// @name 重载基类 algs::StandardComponentPatchStrategy<NDIM> 的函数:
  // @{

  /*!
   * @brief 初始化指定的积分构件.
   *
   * 注册待填充的数据片或待调度内存空间的数据片到积分构件.
   *
   * @param component 输入参数, 指针, 指向待初始化的积分构件对象.
   */
  void initializeComponent(algs::IntegratorComponent<NDIM>* component) const;

  /**
   * @brief 初始化数据片（支持初值构件）.
   *
   * @param patch 输入参数, 网格片类, 表示网格片.
   * @param time  输入参数, 双精度浮点型, 表示初始化的时刻.
   * @param initial_time 输入参数, 逻辑型, 真值表示当前时刻为计算的初始时刻.
   * @param component_name 输入参数, 字符串, 当前调用该函数的初值构件之名称.
   */
  void initializePatchData(hier::Patch<NDIM>& patch, const double time,
                           const bool initial_time,
                           const string& component_name);
  /*!
   * @brief 完成单个网格片上的数值计算（支持数值构件）.
   *
   * @param patch 输入参数, 网格片类, 表示网格片.
   * @param time  输入参数, 双精度浮点型, 表示当前时刻.
   * @param dt    输入参数, 双精度浮点型, 表示时间步长.
   * @param initial_time  输入参数, 逻辑型, 表示当前是否为初始时刻.
   * @param component_name 输入参数, 字符串, 当前调用该函数的数值构件之名称.
   */
  void computeOnPatch(hier::Patch<NDIM>& patch, const double time,
                      const double dt, const bool initial_time,
                      const string& component_name);

  /*!
   * @brief 完成单个网格片上的通量计算.
   *
   * @param patch 输入参数, 网格片类, 表示网格片.
   * @param time  输入参数, 双精度浮点型, 表示当前时刻.
   * @param dt    输入参数, 双精度浮点型, 表示时间步长.
   * @param initial_time  输入参数, 逻辑型, 表示当前是否为初始时刻.
   * @param component_name 输入参数, 字符串, 当前调用该函数的数值构件之名称.
   */
  void computeFlux(hier::Patch<NDIM>& patch, const double time,
                          const double dt, const bool initial_time,
                          const string& component_name);

  /*!
   * @brief 完成单个网格片上的守恒量计算.
   *
   * @param patch 输入参数, 网格片类, 表示网格片.
   * @param time  输入参数, 双精度浮点型, 表示当前时刻.
   * @param dt    输入参数, 双精度浮点型, 表示时间步长.
   * @param initial_time  输入参数, 逻辑型, 表示当前是否为初始时刻.
   * @param component_name 输入参数, 字符串, 当前调用该函数的数值构件之名称.
   */
  void computevalue(hier::Patch<NDIM>& patch, const double time,
                                  const double dt, const bool initial_time,
                                  const string& component_name);

 
  

  /*!
   * @brief 在单个网格片上计算稳定性时间步长（支持时间步长构件）.
   *
   * @param patch 输入参数, 网格片类, 表示网格片.
   * @param time  输入参数, 双精度浮点型, 表示初始化的时刻.
   * @param initial_time 输入参数, 逻辑型, 真值表示当前时刻为计算的初始时刻.
   * @param flag_last_dt   输入参数, 整型, 表示前次积分返回的状态.
   * @param last_dt 输入参数, 双精度浮点型, 表示前次积分的时间步长.
   * @param component_name 输入参数, 字符串, 当前调用该函数的时间步长构件之名称.
   *
   * @return 双精度浮点型, 时间步长.
   */
  double getPatchDt(hier::Patch<NDIM>& patch, const double time,
                    const bool initial_time, const int flag_last_dt,
                    const double last_dt, const string& component_name);

  
  
  //@}

  ///@name 自定义函数
  //@{

  /*!
   * @brief 注册绘图量.
   * @param javis_writer 输入参数, 指针, 表示 JaVis 数据输出器.
   */
  void registerPlotData(
      tbox::Pointer<appu::JaVisDataWriter<NDIM> > javis_writer);

private:

  /*!@brief 注册变量和数据片.  */
  void registerModelVariables();

  
  int d_uval_current_id;  /*当前守衡量*/
  int d_uval_new_id;      /*新值守恒量*/
  int d_uval_scratch_id;  /*演算守恒量*/
  int d_flux_new_id;      /*通量f1 */
  int d_add_new_id;       /*修正量add */
 
  int d_gw;               /*影像区宽度 */
};


#endif