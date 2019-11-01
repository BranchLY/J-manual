
#include "GridGeometry.h"
#include "GridTopology.h"
#include "PatchHierarchy.h"
#include "HierarchyTimeIntegrator.h"
#include "JaVisDataWriter.h"
#include "JAUMINManager.h"
#include "InputManager.h"
#include "VariableDatabase.h"
#include "Utilities.h"
#include "CompareData.h"
using namespace JAUMIN;

#include "PoissonLevel.h"
#include "Poisson.h"

/*!
*************************************************************************
* @brief   在网格文件名前面追加input文件的路径
************************************************************************
*/
static void prefixInputDirName(const string& input_filename,
                               tbox::Pointer<tbox::Database> input_db) {
  string path_name = "";
  string::size_type slash_pos = input_filename.find_last_of('/');
  if (slash_pos != string::npos)
    path_name = input_filename.substr(0, slash_pos + 1);

  string mesh_file = input_db->getDatabase("GridGeometry")
                         ->getDatabase("MeshImportationParameter")
                         ->getString("file_name");

  slash_pos = mesh_file.find_first_of('/');
  if (slash_pos != 0) {
    input_db->getDatabase("GridGeometry")
        ->getDatabase("MeshImportationParameter")
        ->putString("file_name", path_name + mesh_file);
  }
}



int main(int argc, char* argv[]) {
  // 初始化MPI和JAUMIN环境.
  tbox::MPI::init(&argc, &argv);
  tbox::JAUMINManager::startup();
  {
    /*******************************************************************************
     *                               预  处  理 *
     *******************************************************************************/

    // 解析命令行参数:

    string input_filename;         // 输入文件名.
    
    if ((argc != 2) && (argc != 4)) {
      tbox::pout << "USAGE:  " << argv[0] << " <input filename> "
                 << "<restart dir> <restore number> [options]\n"
                 << "  options:\n"
                 << "  none at this time" << endl;
      tbox::MPI::abort();
      return (-1);
    } else {
      input_filename = argv[1];     // 输入文件名,这里不考虑重启动
    } 

    

    // 解析输入文件的计算参数到输入数据库, 称之为根数据库.
    tbox::Pointer<tbox::Database> input_db =
        new tbox::InputDatabase("input_db");
    tbox::InputManager::getManager()->parseInputFile(input_filename, input_db);

    // 在网格文件名前面追加input文件的路径
    prefixInputDirName(input_filename, input_db);

    // 从根数据库中获得名称为"Main"的子数据库.
    tbox::Pointer<tbox::Database> main_db = input_db->getDatabase("Main");

    // 从"Main"子数据库中获取可视化输出控制参数.
    int javis_dump_interval =
        main_db->getIntegerWithDefault("javis_dump_interval", 0);
    string javis_dump_dirname;
    int javis_number_procs_per_file;
    if (javis_dump_interval > 0) {
      javis_dump_dirname = main_db->getString("javis_dump_dirname");
      javis_number_procs_per_file =
          main_db->getIntegerWithDefault("javis_number_procs_per_file", 1);
    }



    //创建几何
    tbox::Pointer<hier::GridGeometry<NDIM> > grid_geometry =
        new hier::GridGeometry<NDIM>("GridGeometry",
                                     input_db->getDatabase("GridGeometry"));
    //创建拓扑
    tbox::Pointer<hier::GridTopology<NDIM> > grid_topology =
        new hier::GridTopology<NDIM>("GridTopology",
                                     input_db->getDatabase("GridTopology"));

    //创建网格片层次结构.
    tbox::Pointer<hier::PatchHierarchy<NDIM> > patch_hierarchy =
        new hier::PatchHierarchy<NDIM>("PatchHierarchy", grid_geometry,
                                       grid_topology, true);

    /*******************************************************************************
     *                                 算法                                         *
     *******************************************************************************/

    // (1) 网格片，用户定义，采用C++标准指针
    Poisson* Poisson_patch =
        new Poisson("Poisson_patch", input_db->getDatabase("Poisson"));

    // (2) 网格层，用户定义，采用C++标准指针
    PoissonLevel* Poisson_level =
        new PoissonLevel("Poisson_level", Poisson_patch);

    // (3) 时间算法，框架给定，智能指针
    tbox::Pointer<algs::HierarchyTimeIntegrator<NDIM> > time_integrator =
        new algs::HierarchyTimeIntegrator<NDIM>(
            "HierarchyTimeIntegrator",
            input_db->getDatabase("HierarchyTimeIntegrator"), patch_hierarchy,
            Poisson_level, true);

    //创建jaVis类
    tbox::Pointer<appu::JaVisDataWriter<NDIM> > javis_data_writer;
    if (javis_dump_interval > 0) {
      javis_data_writer = new appu::JaVisDataWriter<NDIM>(
          "Euler_JaVis_Writer", javis_dump_dirname,
          javis_number_procs_per_file);

      Poisson_patch->registerPlotData(
          javis_data_writer);  //注册待输出的绘图量.
    }


    /*******************************************************************************
     *                                  计算                                        *
     *******************************************************************************/
    // 初始化
    time_integrator->initializeHierarchy();

    // 输出初始可视化数据场到jaVis
    if (javis_dump_interval > 0) {
      javis_data_writer->writePlotData(time_integrator->getPatchHierarchy(),
                                       time_integrator->getIntegratorStep(),
                                       time_integrator->getIntegratorTime());
    }
    
     //时间步进循环 

    double loop_time = time_integrator->getIntegratorTime();
    double loop_time_end = time_integrator->getEndTime();

    int iteration_num = time_integrator->getIntegratorStep();
 
    while ((loop_time < loop_time_end) && time_integrator->stepsRemaining()) {
      iteration_num = time_integrator->getIntegratorStep() + 1;

      //返回时间步长
      double dt_actual = time_integrator->advanceHierarchy();

      loop_time += dt_actual;

    // 输出可视化数据场到jaVis
      if ((javis_dump_interval > 0) &&
          (((iteration_num % javis_dump_interval) == 0) ||
           !((loop_time < loop_time_end) &&
             time_integrator->stepsRemaining()))) {
        
        javis_data_writer->writePlotData(time_integrator->getPatchHierarchy(),
                                         iteration_num, loop_time);
      }
}
    /************************************************************************************
     *                               模  拟  结  束 *
     ************************************************************************************/
    // 释放对象.
    if (Poisson_patch) delete Poisson_patch;
    if (Poisson_level) delete Poisson_level;
  }
  // 释放JAUMIN和MPI内部资源.
  tbox::JAUMINManager::shutdown();
  tbox::MPI::finalize();

  return (0);
}
