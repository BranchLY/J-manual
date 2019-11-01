#include <assert.h>

#include "Patch.h"                //网格片类
#include "PatchTopology.h"   //网格片拓扑类
#include "PatchGeometry.h"  //网格片几何类
#include "EntityUtilities.h"     //实例集
#include "IntegratorComponent.h"//构件基类

// 创建数据片中的变量 需要的头文件
#include "VectorVariable.h"  //向量
#include "NodeVariable.h"   //点心量
#include "CSRMatrixVariable.h"//矩阵

// 获取压缩稀疏行矩阵数据片  需要的头文件
#include "CSRMatrixData.h" //矩阵数据片

#include "Matrix.h" //矩阵

#include "math.h"
#include "PatchCompute.h"
using namespace JAUMIN;

// 构造函数：
PatchCompute::PatchCompute(
    const string &object_name,                // 对象名称
    tbox::Pointer<tbox::Database> Possion_db) // 自定义数据库Possion{}
{
    /******************************创建变量（4.1节）*************************************/
    // 创建自由度信息，参数信息（四个逻辑型表示点，边，面，体上自由度是否非零）。
    d_dof_info = new solv::DOFInfo<NDIM>(true, false, false, false);
    // 向量变量， 解向量
    tbox::Pointer<pdat::VectorVariable<NDIM, double> > solution =
        new pdat::VectorVariable<NDIM, double>("solution", d_dof_info);
    // 向量变量， 右端项
    tbox::Pointer<pdat::VectorVariable<NDIM, double> > rhs =
        new pdat::VectorVariable<NDIM, double>("rhs", d_dof_info);
    // 矩阵变量， 矩阵
    tbox::Pointer<pdat::CSRMatrixVariable<NDIM, double> > matrix =
        new pdat::CSRMatrixVariable<NDIM, double>("matrix", d_dof_info);
    //点心量变量，绘图
    tbox::Pointer<pdat::NodeVariable<NDIM, double> > plotdata =
        new pdat::NodeVariable<NDIM, double>("plot", 1);

    /******************************创建上下文（4.2节）*************************************/
    // 获取变量管理器（管理 变量、上下文、<变量，上下文 >与数据片索引号之间的映射关系）
    hier::VariableDatabase<NDIM> *var_db = hier::VariableDatabase<NDIM>::getDatabase();
    //创建上下文
    tbox::Pointer<hier::VariableContext> current =
        var_db->getContext("CURRENT");

    /******************************创建数据片，获取数据片ID（4.3节）*************************************/
    // 将变量及上下文注册到变量数据库.返回或分配数据片<Varibale,Context>，获得数据片索引
    d_solution_id = var_db->registerVariableAndContext(solution, current, 1); //解向量数据片ID
    d_rhs_id = var_db->registerVariableAndContext(rhs, current, 1);           //右端向量数据片ID
    d_matrix_id = var_db->registerVariableAndContext(matrix, current, 1);     //左端矩阵数据片ID
    d_plot_id = var_db->registerVariableAndContext(plotdata, current, 1);     //绘图数据片ID
}

// 析构函数：
PatchCompute::~PatchCompute() {}

//注册数据片到初值构件或内存构件
void PatchCompute::initializeComponent(
    algs::IntegratorComponent<NDIM> *integ_comp) const // 构件的基类，会传入各类型构件
{
     /******************************注册数据片到构件（5.3节）*************************************/
    const string &comp_name = integ_comp->getName();

    if (comp_name == "INIT_VALUES")
    { // 为初值构件 注册待初始化的数据片
        integ_comp->registerInitPatchData(d_plot_id);
        // 将自由度信息中的若干数据片注册到初始化构件
        d_dof_info->registerToInitComponent(integ_comp);
    }
    else if (comp_name == "STEP_SIZE")
    { // 步长构件只访问数据片，根据算法决定是否需要局部通信
    }
    else if (comp_name == "CMP_EQUATION")
    { // 为数值构件 注册待局部通信的数据片及填充方法
    }
    else if (comp_name == "CMP_POST")
    { // 为数值构件 注册待局部通信的数据片及填充方法
    }
    else if (comp_name == "ALLOC")
    { // 为内存构件 注册中间量数据片（框架内部实现，此次说明对什么数据片操作）
        integ_comp->registerPatchData(d_solution_id);
        integ_comp->registerPatchData(d_rhs_id);
        integ_comp->registerPatchData(d_matrix_id);
    }
    else
    {
        TBOX_ERROR(" PatchStrategy :: component name is error! ");
    }
}

//初始化数据片
void PatchCompute::initializePatchData(
    hier::Patch<NDIM> &patch,     // 待初始化数据的的网格片
    const double time,            // 初始化的时刻
    const bool initial_time,      // 真值表示当前时刻为计算的初始时刻
    const string &init_comp_name) // 初值构件的名称
{
     /******************************初始化数据片（5.4.1节）*************************************/
    if (initial_time)
    {
        int num_nodes = patch.getNumberOfNodes(1); // 获取网格片内部和前1层影像区中的结点总数
        
        //获取可视化数据片，并初始化
        tbox::Pointer<pdat::NodeData<NDIM, double> > plot_data =
            patch.getPatchData(d_plot_id);
        double *plot_ptr = plot_data->getPointer(); // 获取数据片数组指针
        for (int i = 0; i < num_nodes; ++i)
        {
            *(plot_ptr + i) = 0; //所有结点上的值初始化为0
        }

        //获取自由度分布数组
        int *DOF_distribution_ptr = d_dof_info->getDOFDistribution(patch, hier::EntityUtilities::NODE);
        // 对自由度信息的分布信息初始化  每个结点的自由度均为1
        for (int i = 0; i < num_nodes; ++i)
        {
            DOF_distribution_ptr[i] = 1;
        }
        // 建立网格片自由度的局部映射信息
        d_dof_info->buildPatchDOFMapping(patch);
    }
}

//设置网格片时间步长
double PatchCompute::getPatchDt( // 返回指定网格片上计算的时间步长
    hier::Patch<NDIM> &patch,    // 待计算时间步长的网格片
    const double time,           // 当前时刻
    const bool initial_time,     // 真值表示当前时刻为计算的初始时刻
    const int flag_last_dt,      // 上个时间步积分返回的状态
    const double last_dt,        // 上个时间步的时间步长
    const string &dt_comp_name)  // 步长构件的名称
{
#ifdef DEBUG_CHECK_ASSERTIONS // 参数检查
    assert(dt_comp_name == "STEP_SIZE");
#endif
 /******************************计算步长（5.4.3节）*************************************/
    
    /********************************************************************
     *                                                             编写网格片的时间步长算法
     ********************************************************************/
    return 0.1; //此处返回0.01
}

//网格片数值计算
void PatchCompute::computeOnPatch(
    hier::Patch<NDIM> &patch, // 待计算的网格片
    const double time,        // 当前时刻
    const double dt,          // 时间步长
    const bool initial_time,  // 真值表示当前时刻为计算的初始时刻
    const string &comp_name)  // 数值构件的名称
{
    if (comp_name == "CMP_EQUATION")
    {
        buildMatrixOnPatch(patch, time, dt, comp_name); //计算左端矩阵
        buildRHSOnPatch(patch, time, dt, initial_time, comp_name);//计算右端向量
        applyBoundary(patch, time, dt, comp_name);//通过边界条件修正矩阵与向量
    }
    else if (comp_name == "CMP_POST")
    {
        postProcess(patch, time, dt, comp_name);//将解向量值传给绘图数据片
    }
    else
    {
        TBOX_ERROR(" PatchStrategy :: component name is error! ");
    }
}

//计算左端向矩阵
void PatchCompute::buildMatrixOnPatch(
    hier::Patch<NDIM> &patch,    // 待计算的网格片
    const double time,           // 当前时刻
    const double dt,             // 时间步长
    const string &num_comp_name) // 数值构件的名称
{
    // 获取左端矩阵数据片
    tbox::Pointer<pdat::CSRMatrixData<NDIM, double> > mat_data =
        patch.getPatchData(d_matrix_id);
    // 获取网格片几何信息
    tbox::Pointer<hier::PatchGeometry<NDIM> > patch_geo =
        patch.getPatchGeometry();
    // 获取网格片拓扑信息.
    tbox::Pointer<hier::PatchTopology<NDIM> > patch_top =
        patch.getPatchTopology();
    // 通过网格片几何信息获取结点坐标数据片，并获取数组指针
    tbox::Pointer<pdat::NodeData<NDIM, double> > node_coord =
        patch_geo->getNodeCoordinates();
    double *node_coord_ptr = node_coord->getPointer();

    tbox::Array<int> cell_node_extent, cell_node_indices;                  // 定义邻接表压缩存储的位置数组和索引数组
    patch_top->getCellAdjacencyNodes(cell_node_extent, cell_node_indices); // 获取网格片<单元，结点>邻接拓扑关系

    // 取出自由度映射数组首地址，存储每个点上的起始自由度编号
    int *dof_map = d_dof_info->getDOFMapping(patch, hier::EntityUtilities::NODE);
    // 通过单元矩阵映射整合计算赋值整体矩阵数据片mat_data

    int num_cells = patch.getNumberOfCells(1); //获取网格片及1层影像区上的单元数目
    tbox::Matrix<double> dN_dx; //形函数x偏导
    tbox::Matrix<double> dN_dy;//形函数y偏导
    tbox::Matrix<double> EK1;//矩阵
    tbox::Matrix<double> EK2;//矩阵
    tbox::Matrix<double> EK;  //单元矩阵
    dN_dx.resize(1, 3);
    dN_dy.resize(1, 3);
    EK1.resize(3, 3);
    EK2.resize(3, 3);
    EK.resize(3, 3);
    double x[3], y[3], dof[3],b[3],c[3];
    for (int cell_id = 0; cell_id < num_cells; ++cell_id)
    {
        //获取单元上每个结点的x,y坐标
        for (int i = 0; i < cell_node_extent[cell_id + 1] - cell_node_extent[cell_id]; ++i)
        {
           int  node_id = cell_node_indices[i + cell_node_extent[cell_id]];
            x[i] = *(node_coord_ptr + 2 * node_id);     //x坐标
            y[i] = *(node_coord_ptr + 2 * node_id + 1); //y坐标
            //获取自由度索引
            dof[i] = dof_map[node_id];
        }
        b[0] = y[1] - y[2];
        b[1] = y[2] - y[0];
        b[2] = y[0] - y[1];
        c[0] = x[2] - x[1];
        c[1] = x[0] - x[2];
        c[2] = x[1] - x[0];
        double deta = 0.5 * (b[0] * c[1] - b[1] * c[0]);
        dN_dx(0, 0) = 1 / (2 * deta) * b[0];
        dN_dx(0, 1) = 1 / (2 * deta) * b[1];
        dN_dx(0, 2) = 1 / (2 * deta) * b[2];
        dN_dy(0, 0) = 1 / (2 * deta) * c[0];
        dN_dy(0, 1) = 1 / (2 * deta) * c[1];
        dN_dy(0, 2) = 1 / (2 * deta) * c[2];
        EK1 = dN_dx.getTranspose() * dN_dx;
        EK2 = dN_dy.getTranspose() * dN_dy;
        EK = EK1 + EK2;
        for (int t1 = 0; t1 < 3; ++t1)
        {
            for (int t2 = 0; t2 < 3; ++t2)
            {
                //组装单元的左端矩阵到总体矩阵中
                mat_data->addMatrixValue(dof[t1], dof[t2], deta * EK(t1, t2));
            }
        }
    }
}

//计算右端项向量
void PatchCompute::buildRHSOnPatch(
    hier::Patch<NDIM> &patch,    // 待计算的网格片
    const double time,           // 当前时刻
    const double dt,             // 时间步长
    const bool initial_time,     // 真值表示当前时刻为计算的初始时刻
    const string &num_comp_name) // 数值构件的名称
{
    // 获取右端向量数据片
    tbox::Pointer<pdat::VectorData<NDIM, double> > vec_data =
        patch.getPatchData(d_rhs_id);
    // 获取网格片几何信息
    tbox::Pointer<hier::PatchGeometry<NDIM> > patch_geo =
        patch.getPatchGeometry();
    // 获取网格片拓扑信息.
    tbox::Pointer<hier::PatchTopology<NDIM> > patch_top =
        patch.getPatchTopology();
    // 通过网格片几何信息获取结点坐标数据片，并获取数组指针
    tbox::Pointer<pdat::NodeData<NDIM, double> > node_coord =
        patch_geo->getNodeCoordinates();
    double *node_coord_ptr = node_coord->getPointer();

    tbox::Array<int> cell_node_extent, cell_node_indices;                  // 定义邻接表压缩存储的位置数组和索引数组
    patch_top->getCellAdjacencyNodes(cell_node_extent, cell_node_indices); // 获取网格片<单元，结点>邻接拓扑关系

    // 取出自由度映射数组首地址，存储每个点上的起始自由度编号
    int *dof_map = d_dof_info->getDOFMapping(patch, hier::EntityUtilities::NODE);
    // 通过单元矩阵映射整合计算赋值整体矩阵数据片mat_data
    int num_cells = patch.getNumberOfCells(1);
    tbox::Matrix<double> M;  //矩阵
    tbox::Matrix<double> f;   //矩阵
    tbox::Matrix<double> EF; //矩阵
    M.resize(3, 3);
    f.resize(3, 1);
    EF.resize(3, 1);
    double x[3], y[3], dof[3],c[3],b[3];
    for (int cell_id = 0; cell_id < num_cells; ++cell_id)
    {
        //获取单元上每个结点的x,y坐标
        for (int i = 0; i < cell_node_extent[cell_id + 1] - cell_node_extent[cell_id]; ++i)
        {
           int node_id = cell_node_indices[i + cell_node_extent[cell_id]];
            x[i] = *(node_coord_ptr + 2 * node_id);     //x坐标
            y[i] = *(node_coord_ptr + 2 * node_id + 1); //y坐标
            //获取自由度索引
            dof[i] = dof_map[node_id];
        }
        b[0] = y[1] - y[2];
        b[1] = y[2] - y[0];
        b[2] = y[0] - y[1];
        c[0] = x[2] - x[1];
        c[1] = x[0] - x[2];
        c[2] = x[1] - x[0];
        double deta = 0.5 * (b[0] * c[1] - b[1] * c[0]);
        for (int j = 0; j < 3; ++j)
        {
            f(j, 0) = -2.0 * (x[j] * x[j] - x[j]) - 2.0 * (y[j] * y[j] - y[j]);
        }
        M(0, 0) = 2.0 * deta / 12.0;
        M(0, 1) = deta / 12.0;
        M(0, 2) = deta / 12.0;
        M(1, 0) = deta / 12.0;
        M(1, 1) = 2.0 * deta / 12.0;
        M(1, 2) = deta / 12.0;
        M(2, 0) = deta / 12.0;
        M(2, 1) = deta / 12.0;
        M(2, 2) = 2.0 * deta / 12.0;
        EF = M * f;
        for (int t1 = 0; t1 < 3; ++t1)
        {
            //组装单元的右端向量到总体向量中
            vec_data->addVectorValue(dof[t1], EF(t1, 0));
        }
    }
}

//通过边界条件修正矩阵与向量
void PatchCompute::applyBoundary(
    hier::Patch<NDIM> &patch,    // 待计算的网格片
    const double time,           // 当前时刻
    const double dt,             // 时间步长
    const string &num_comp_name) // 数值构件的名称
{
    // 获取左端矩阵数据片
    tbox::Pointer<pdat::CSRMatrixData<NDIM, double> > mat_data =
        patch.getPatchData(d_matrix_id);
    // 获取右端向量数据片
    tbox::Pointer<pdat::VectorData<NDIM, double> > vec_data =
        patch.getPatchData(d_rhs_id);
    // 获取网格片几何信息
    tbox::Pointer<hier::PatchGeometry<NDIM> > patch_geo =
        patch.getPatchGeometry();
    // 取出自由度映射数组首地址，存储每个点上的起始自由度编号
    int *dof_map = d_dof_info->getDOFMapping(patch, hier::EntityUtilities::NODE);
    //判断是否存在编号为1的点类型实体集
    if(patch_geo->hasEntitySet(1, hier::EntityUtilities::NODE)==1){
        //获取实体集上的实体编号
        const tbox::Array<int>& bdry_id_set = patch_geo->getEntityIndicesInSet(1, hier::EntityUtilities::NODE);
    for (int node_id = 0; node_id < bdry_id_set.size(); ++node_id)
    {
        //获取自由度索引
        int dof = dof_map[bdry_id_set[node_id]];
        for (int i = 0; i < mat_data->get_num_rows(); ++i)
        {
            mat_data->setMatrixValue(dof, i, 0.0);
            mat_data->setMatrixValue(i, dof, 0.0);
            vec_data->setVectorValue(dof, 0.0);
        }
        mat_data->setMatrixValue(dof, dof, 1.0);
    }
    }
   //将链表形式矩阵转换为CSR格式
    mat_data->assemble();
}

//填充绘图数据片
void PatchCompute::postProcess(
    hier::Patch<NDIM> &patch,    // 待计算的网格片
    const double time,           // 当前时刻
    const double dt,             // 时间步长
    const string &num_comp_name) // 数值构件的名称
{
    //获取解向量数据片
    tbox::Pointer<pdat::VectorData<NDIM, double> > sol_data =
        patch.getPatchData(d_solution_id);
    //获取绘图数据片
    tbox::Pointer<pdat::NodeData<NDIM, double> > plot_data =
        patch.getPatchData(d_plot_id);
    //获取数组指针
    double *plot_data_ptr = plot_data->getPointer();
    double *sol_data_ptr = sol_data->getPointer();
    int num_nodes = patch.getNumberOfNodes(1);
    //将解向量的值附给绘图数据片
    for (int i = 0; i < num_nodes; ++i)
    {
        *(plot_data_ptr + i) = *(sol_data_ptr + i);
    }
}
