#ifndef PATCHCOMPUTE_H
#define PATCHCOMPUTE_H

#include <StandardComponentPatchStrategy.h> // 标准构件网格片积分算法策略基类
#include "DOFInfo.h"
#include <JaVisDataWriter.h>

using namespace JAUMIN;

/**
 * @brief 用户定义的网格片积分算法类，继承自抽象基类StandardComponentPatchStrategy
 * 必须实现其基类的纯虚函数(被框架调用)
 *
 * 该类的接口函数分别由 初值/步长/数值/内存/复制/等 积分构件调用，具体实现了物理问题的数值计算
 * 具体接口在重构时通常是调用Fortran子程序实现
 */
class PatchCompute : public algs::StandardComponentPatchStrategy<NDIM>
{

public:
    // =======构造函数：
    PatchCompute(
        const string &object_name,                 // 对象名称
        tbox::Pointer<tbox::Database> Possion_db); // 便于修改的数值计算参数组成的自定义数据库

    // =======析构函数：
    virtual ~PatchCompute(); // 虚析构函数，确保delete一个指向派生类的基类指针可以执行正确的析构函数版本

    // =======注册数据片到初值构件或内存构件
    void initializeComponent(
        algs::IntegratorComponent<NDIM> *integ_comp) const; // 指向待初始化的积分构件对象

    // =======初始化被注册在初值构件的数据片
    void initializePatchData(
        hier::Patch<NDIM> &patch,      // 待初始化数据的的网格片
        const double time,             // 初始化的时刻
        const bool initial_time,       // 真值表示当前时刻为计算的初始时刻
        const string &init_comp_name); // 初值构件的名称

    // =======设置网格片时间步长
    double getPatchDt(               // 返回指定网格片上计算的时间步长
        hier::Patch<NDIM> &patch,    // 待计算时间步长的网格片
        const double time,           // 当前时刻
        const bool initial_time,     // 真值表示当前时刻为计算的初始时刻
        const int flag_last_dt,      // 上个时间步积分返回的状态
        const double last_dt,        // 上个时间步的时间步长
        const string &dt_comp_name); // 步长构件的名称

    // =======网格片数值计算
    void computeOnPatch(
        hier::Patch<NDIM> &patch, // 待计算的网格片
        const double time,        // 当前时刻
        const double dt,          // 时间步长
        const bool initial_time,  // 真值表示当前时刻为计算的初始时刻
        const string &comp_name); // 数值构件的名称


    // =======查询函数
    int getMatrixID() { return d_matrix_id; }                              // 获取矩阵id
    int getRHSID() { return d_rhs_id; }                                    //  获取右端项i
    int getSolutionID() { return d_solution_id; }                          // 获取解向量id
    tbox::Pointer<solv::DOFInfo<NDIM> > getDOFInfo() { return d_dof_info; } // 获取自由度信息
    int d_plot_id;     // 绘图数据片ID，便于主函数获取，设为公有
private:

    // ***** 私有函数，在单个网格片上完成矩阵组装. computeOnPatch中使用，支撑指定名称的数值构件
    void buildMatrixOnPatch(
        hier::Patch<NDIM> &patch, // 待计算的网格片
        const double time,        // 当前时刻
        const double dt,          // 时间步长
        const string &comp_name); // 数值构件的名称

    // ***** 私有函数，在单个网格片上完成右端项组装. computeOnPatch中使用，支撑指定名称的数值构件
    void buildRHSOnPatch(
        hier::Patch<NDIM> &patch, // 待计算的网格片
        const double time,        // 当前时刻
        const double dt,          // 时间步长
        const bool initial_time,  // 真值表示当前时刻为计算的初始时刻
        const string &comp_name); // 数值构件的名称

    // ***** 私有函数，在单个网格片上完成约束边条加载. computeOnPatch中使用，支撑指定名称的数值构件
    void applyBoundary(
        hier::Patch<NDIM> &patch, // 待计算的网格片
        const double time,        // 当前时刻
        const double dt,          // 时间步长
        const string &comp_name); // 数值构件的名称

    // ***** 私有函数，在单个网格片上完成后处理计算. computeOnPatch中使用，支撑指定名称的数值构件
    void postProcess(
        hier::Patch<NDIM> &patch, // 待计算的网格片
        const double time,        // 当前时刻
        const double dt,          // 时间步长
        const string &comp_name); // 数值构件的名称

private:
    // 对象名称
    string d_object_name;

    // 自由度信息
    // 包含 DOFDistribution数组: 存储每个点、边、面、体上有几个自由度
    //     DOFMapping数组：存储每个点、边、面、体上的起始自由度编号
    tbox::Pointer<solv::DOFInfo<NDIM> > d_dof_info;

    //数据片ID
    int d_solution_id; //解向量数据片ID
    int d_matrix_id;   //左端矩阵数据片ID
    int d_rhs_id;       //右端向量数据片ID
};

#endif
