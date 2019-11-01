
#include "PatchGeometry.h"
#include "PatchTopology.h"
#include "CellData.h"
#include "EdgeData.h"
#include "EdgeVariable.h"
#include "CellVariable.h"
#include "Database.h"
#include "fort.h"
#include "Poisson.h"
/*************************************************************************
 *                             构造函数.                                  *
 *************************************************************************/
Poisson::Poisson(const string& object_name,
             tbox::Pointer<tbox::Database> input_db) {
#ifdef DEBUG_CHECK_ASSERTIONS
  TBOX_ASSERT(!object_name.empty());
  TBOX_ASSERT(!input_db.isNull());
#endif
  // 注册变量和数据片.
  registerModelVariables();
}


/*********注册变量和数据片************/

void Poisson::registerModelVariables() {
  // 变量数据库.
  hier::VariableDatabase<NDIM>* variable_db =
      hier::VariableDatabase<NDIM>::getDatabase();

  // 定义变量.守衡量u，通量f1（直接梯度项），add修正
  tbox::Pointer<pdat::CellVariable<NDIM, double> >d_uval = 
    new pdat::CellVariable<NDIM, double>("uval",1);  // 守衡量u，中心量, 群数为1，深度默认为1
  tbox::Pointer<pdat::EdgeVariable<NDIM, double> >d_flux = 
    new pdat::EdgeVariable<NDIM, double>("flux",1);  // 通量f1，边心量, 群数为1，深度默认为1
  tbox::Pointer<pdat::EdgeVariable<NDIM, double> >d_add = 
    new pdat::EdgeVariable<NDIM, double>("Add",1);  // 修正add，边心量, 群数为1，深度默认为1
  // 当前值current上下文, 新值new上下文, 演算scratch上下文.
  tbox::Pointer<hier::VariableContext>d_current =
                             variable_db->getContext("CURRENT");
  tbox::Pointer<hier::VariableContext>d_new = 
                             variable_db->getContext("NEW");
  tbox::Pointer<hier::VariableContext>d_scratch = 
                            variable_db->getContext("SCRATCH");
  //影像区宽度d_gw设为1，创建数据片
  d_gw=1;
  d_uval_current_id =                    //current时间步守衡量u，存储当前时刻网格片上的值
        variable_db->registerVariableAndContext(d_uval,d_current);
  d_uval_new_id =                        //new时间步守衡量u，存储新时刻网格片上的值
        variable_db->registerVariableAndContext(d_uval,d_new);
  d_uval_scratch_id =                    //scratch时间步守衡量u，参与计算，需要用到相邻网格片上的u值，设立一层影像区
        variable_db->registerVariableAndContext(d_uval,d_scratch,d_gw);
  d_flux_new_id = variable_db->registerVariableAndContext(d_flux,d_new);  //通量f1
  d_add_new_id = variable_db->registerVariableAndContext(d_add,d_new);    //修正add
} 


/*************************************************************************
 *                             析构函数.                                  *
 ************************************************************************/
Poisson::~Poisson() {
};




/*************************************************************************
 *                         数据片注册到构件                                *
 ************************************************************************/
void Poisson::initializeComponent(
    algs::IntegratorComponent<NDIM>* component) const {
#ifdef DEBUG_CHECK_ASSERTIONS
  TBOX_ASSERT(component);
#endif
  //获取构件名称，用于辨别不同构件，注册相对应的数据片
  const string& component_name = component->getName();

  if (component_name == "INIT_SET_VALUE") {  // 初值构件，分配current数据片内存和赋初值
    component->registerInitPatchData(d_uval_current_id);
  } else if (component_name == "ALLOC_NEW_DATA") {  // 内存构件
    component->registerPatchData(d_uval_new_id);
    component->registerPatchData(d_flux_new_id);
    component->registerPatchData(d_add_new_id);
  } else if (component_name == "ALLOC_SCRATCH_DATA") {  // 内存构件
    component->registerPatchData(d_uval_scratch_id);
  } else if (component_name == "Dt") {        // 步长构件
  } else if (component_name == "COMPUTING") { // 数值构件, 更新守衡量u
  //数值构件, 计算通量f1和修正量add，这里由于scratch的值是在current的基础上扩一层影像区得到，二者存在数据填充关系，故将数据片ID作为参数给出
  } else if (component_name == "COMPUTE_FLUX") { 
  component->registerCommunicationPatchData(d_uval_scratch_id,d_uval_current_id);   
  } else if (component_name == "COPY_SOLUTION") {   //复制构件，时间推进，新值作为下一时间步的current值，存在数据填充关系
  component->registerCopyPatchData(d_uval_current_id,d_uval_new_id);
  } else {
    TBOX_ERROR("\n::initializeComponent() : component "
               << component_name << " is not matched. " << endl);
  }
}



/*************************************************************************
 *                网格片上初始化数据片（由初值构件调用）                       *
 ************************************************************************/
void Poisson::initializePatchData(hier::Patch<NDIM>& patch, const double time,
                                const bool initial_time,
                                const string& component_name) {
#ifdef DEBUG_CHECK_ASSERTIONS
  TBOX_ASSERT(component_name == "INIT_SET_VALUE");
#endif
  NULL_USE(time); /**< 初始化中没有用到time */

  if (initial_time) {
      int num_cells=patch.getNumberOfCells();
    //获取current守恒量u数据片.
    tbox::Pointer<pdat::CellData<NDIM, double> > uval=
        patch.getPatchData(d_uval_current_id);
    //调用fortran程序为current守衡量u赋初值，传递首地址和长度（u是中心量，长度与单元数一致，current不参与计算，影像区为0）
    init_(uval->getPointer(),num_cells);
 
  }
}



/*************************************************************************
 *                  网格片上计算时间步长（由步长构件调用）                     *
 ************************************************************************/
double Poisson::getPatchDt(hier::Patch<NDIM>& patch, const double time,
                         const bool initial_time, const int flag_last_dt,
                         const double last_dt, const string& component_name) {
  //返回时间步长
  return 0.00001;
}



/*************************************************************************
 *                 网格片上的计算一个时间步（由数值构件调用）                   *
 ************************************************************************/
void Poisson::computeOnPatch(hier::Patch<NDIM>& patch, const double time,
                           const double dt, const bool initial_time,
                           const string& component_name) {
  //通过构件名称确定具体进行的操作
  if (component_name == "COMPUTING") {
    computevalue(patch, time, dt, initial_time, component_name);
  } else if (component_name == "COMPUTE_FLUX") {
    computeFlux(patch, time, dt, initial_time, component_name);
     
  }  
   
}

/*********计算通量************/
void Poisson::computeFlux(hier::Patch<NDIM>& patch, const double time,
                               const double dt, const bool initial_time,
                               const string& component_name) {
  //待操作的数据片
  tbox::Pointer<pdat::CellData<NDIM, double> > uval_scratch=
        patch.getPatchData(d_uval_scratch_id);
  tbox::Pointer<pdat::EdgeData<NDIM, double> > flux_new=
        patch.getPatchData(d_flux_new_id);
  tbox::Pointer<pdat::EdgeData<NDIM, double> > add_new=
        patch.getPatchData(d_add_new_id);
  //获取网格相关的数目
  int num_extent_cells = patch.getNumberOfCells(d_gw);     //注意影像区，要保证网格片内（不含影像区部分）的计算是准确的
  int num_extent_nodes = patch.getNumberOfNodes(d_gw);     //需要用到的点和单元的信息必须有影像区
  int num_edges = patch.getNumberOfEdges();                //通量在边上，只须网格片内的计算准确，故不必包含影像区
  // 获取几何和拓扑
  tbox::Pointer<hier::PatchGeometry<NDIM> > patch_geo =
      patch.getPatchGeometry();
  tbox::Pointer<hier::PatchTopology<NDIM> > patch_top =
      patch.getPatchTopology();
  //由几何获得坐标
  tbox::Pointer<pdat::CellData<NDIM, double> > cell_coords=
        patch_geo->getCellCoordinates();
  tbox::Pointer<pdat::NodeData<NDIM, double> > node_coords=
        patch_geo->getNodeCoordinates();
  //由拓扑获取<边, 单元>拓扑关系.
  tbox::Array<int> eac_extent, eac_index;
  patch_top->getEdgeAdjacencyCells(eac_extent,eac_index);
  //由拓扑获取<边, 结点>拓扑关系.
  tbox::Array<int> ean_extent, ean_index;
  patch_top->getEdgeAdjacencyNodes(ean_extent, ean_index);
  //由拓扑获取<结点, 单元>拓扑关系.用于修正项
  tbox::Array<int> nac_extent, nac_index;
  patch_top->getNodeAdjacencyCells(nac_extent, nac_index);

  //Fortran程序接口求通量f1和修正量add
  comflux_(uval_scratch->getPointer(),flux_new->getPointer(),add_new->getPointer(),
      cell_coords->getPointer(),node_coords->getPointer(),
      eac_extent.getPointer(),eac_index.getPointer(),
      ean_extent.getPointer(),ean_index.getPointer(),
      nac_extent.getPointer(),nac_index.getPointer(),
      num_edges,num_extent_cells,
      num_extent_nodes );

}

/*********更新守恒量u************/
void Poisson::computevalue(hier::Patch<NDIM>& patch,
                                       const double time, const double dt,
                                       const bool initial_time,
                                       const string& component_name) {
  tbox::Pointer<pdat::CellData<NDIM, double> > uval_current=
        patch.getPatchData(d_uval_current_id);
  tbox::Pointer<pdat::CellData<NDIM, double> > uval_new=
        patch.getPatchData(d_uval_new_id);
  tbox::Pointer<pdat::EdgeData<NDIM, double> > flux_new=
        patch.getPatchData(d_flux_new_id);
  tbox::Pointer<pdat::EdgeData<NDIM, double> > add_new=
        patch.getPatchData(d_add_new_id);
  //获取网格相关的数目
  int num_cells = patch.getNumberOfCells();
  int num_edges = patch.getNumberOfEdges();
  // 获取几何和拓扑
  tbox::Pointer<hier::PatchGeometry<NDIM> > patch_geo =
      patch.getPatchGeometry();
  tbox::Pointer<hier::PatchTopology<NDIM> > patch_top =
      patch.getPatchTopology();
  //由几何获得坐标
  tbox::Pointer<pdat::CellData<NDIM, double> > cell_coords=
        patch_geo->getCellCoordinates();
  tbox::Pointer<pdat::NodeData<NDIM, double> > node_coords=
        patch_geo->getNodeCoordinates();

  //由拓扑获取<边, 单元>拓扑关系.
  tbox::Array<int> eac_extent, eac_index;
  patch_top->getEdgeAdjacencyCells(eac_extent,eac_index);

  //由拓扑获取<单元, 边>拓扑关系.
  tbox::Array<int> cae_extent, cae_index;
  patch_top->getCellAdjacencyEdges(cae_extent, cae_index);
  //由拓扑获取<单元，结点>拓扑关系.
  tbox::Array<int> can_extent, can_index;
  patch_top->getCellAdjacencyNodes(can_extent, can_index);


  //Fortran程序接口计算新时刻守衡量u
  computeu_(uval_current->getPointer(),uval_new->getPointer(),flux_new->getPointer(),add_new->getPointer(),
      cell_coords->getPointer(),node_coords->getPointer(),
      eac_extent.getPointer(),eac_index.getPointer(),
      cae_extent.getPointer(),cae_index.getPointer(),
      can_extent.getPointer(),can_index.getPointer(),
      num_cells,num_edges,dt);

}

/*************************************************************************
 *                             jaVis注册画图量                            *
 ************************************************************************/
void Poisson::registerPlotData(
    tbox::Pointer<appu::JaVisDataWriter<NDIM> > javis_writer) {
#ifdef DEBUG_CHECK_ASSERTIONS
  TBOX_ASSERT(!(javis_writer.isNull()));
#endif
  // 注册画图变量, 当前时刻单元上的速度和密度值.
  javis_writer->registerPlotQuantity("U", "SCALAR",
                                     d_uval_current_id);
}