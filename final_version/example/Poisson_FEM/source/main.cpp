#include "GridTopology.h" // jasmin中通过全局索引体现
#include "GridGeometry.h" //
#include <HierarchyTimeIntegrator.h>
#include <PatchHierarchy.h>
#include <JaVisDataWriter.h>

#include <JAUMINManager.h>
#include <InputManager.h>
#include <TimerManager.h>
#include <MPI.h>
using namespace JAUMIN;

#include "LevelCompute.h"
#include "PatchCompute.h"

int main(int argc, char *argv[])
{

        /*****************************************************************************************
                                                                                              1.开始
        *****************************************************************************************/
        // 初始化mpi和JAUMIN环境
        tbox::MPI::init(&argc, &argv);
        tbox::JAUMINManager::startup();

        /*****************************************************************************************
                                                                                              2.预处理
                2.1 获取根数据库，读取控制参数；2.2 日志文件控制；2.3 可视化控制；2.4 计时期控制；2.5 网格信息获取
        *****************************************************************************************/
        // 2.1解析命令行参数，获取根数据库，读取.input文件中的控制参数
        if ((argc != 2) && (argc != 4))
        {
                //不重启动--两个参数，重启动--四个参数(+重启动文件路径、重启动时间序号)
                tbox::pout << "USAGE: " << argv[0] << " <input filename>"
                           << "<restart dir> <restore number> [options]\n"
                           << endl;
                tbox::MPI::abort();
                return -1;
        }

        std::string input_filename = argv[1];
        tbox::plog << "input_filename = " << input_filename << endl;                  //.input文件名
        tbox::Pointer<tbox::Database> input_db = new tbox::InputDatabase("input_db"); // 创建根数据库
        tbox::InputManager::getManager()->parseInputFile(input_filename, input_db);   // 获取根数据库
        tbox::Pointer<tbox::Database> main_db = input_db->getDatabase("Main");        //读取Main数据库

        //2.2  获取Main数据库中的日志文件控制参数
        std::string log_file_name = main_db->getString("log_file_name");
        bool log_all_node = main_db->getBoolWithDefault("log_all_node", false);
        if (log_all_node)
        {
                tbox::PIO::logAllNodes(log_file_name); // 为每个进程产生一个日志文件
        }
        else
        {
                tbox::PIO::logOnlyNodeZero(log_file_name); // 仅为0号进程产生一个日志文件
        }

        //2.3  获取Main数据库中的可视化数据场控制参数
        int javis_dump_interval = main_db->getIntegerWithDefault("javis_dump_interval", 0); // 输出时间步间隔，0表示关闭
        std::string javis_dump_dirname;
        int javis_number_procs_per_file=0;
        if (javis_dump_interval > 0)
        {
                javis_dump_dirname = main_db->getString("javis_dump_dirname");                                  // 存储数据场的目录
                javis_number_procs_per_file = main_db->getIntegerWithDefault("javis_number_procs_per_file", 1); // 多少进程共享一个输出文件
        }

        //2.4 计时器控制
        tbox::Pointer<tbox::Database> timer_db = input_db->getDatabase("TimerManager"); //获取计时器数据库
        tbox::TimerManager::createManager(timer_db);                                    // 初始化计时管理器
        tbox::Pointer<tbox::Timer> t_write_javis_data = tbox::TimerManager::getManager()
                                                            ->getTimer("apps::main::write_javis_data"); // 创建计时器

        //2.5 网格信息获取
        // ***** 创建网格拓扑信息类：
        tbox::Pointer<hier::GridTopology<NDIM> > grid_topology = new hier::GridTopology<NDIM>("GridTopology", input_db->getDatabase("GridTopology"));

        // ***** 创建网格几何信息类：
        tbox::Pointer<hier::BaseGridGeometry<NDIM> > grid_geometry = new hier::GridGeometry<NDIM>("GridGeometry", input_db->getDatabase("GridGeometry"));
        // ***** 创建网格片层次结构类，集合了拓扑信息与几何信息：
        tbox::Pointer<hier::PatchHierarchy<NDIM> > patch_hierarchy = new hier::PatchHierarchy<NDIM>("PatchHierarchy", grid_geometry, grid_topology, true);
        /*****************************************************************************************
                                                                                              3. 算法设置
                3.1 网格片算法设置；3.2 网格层算法设置
        *****************************************************************************************/
        // 3.1 创建网格片算法（algs::StandardComponentPatchStrategy<DIM> 的用户派生类）
        PatchCompute *patch_strategy = new PatchCompute("PatchStrategy", input_db->getDatabase("Possion"));

        // 3.2 创建网格层算法（algs::TimeIntegratorLevelStrategy<DIM> 的用户派生类）
        LevelCompute *level_strategy = new LevelCompute("LevelStrategy", patch_strategy, input_db->getDatabase("Solver"));

        // ***** 创建网格层次结构积分算法类
        tbox::Pointer<algs::HierarchyTimeIntegrator<NDIM> > time_integrator = new algs::HierarchyTimeIntegrator<NDIM>(
            "HierarchyTimeIntegrator",
            input_db->getDatabase("HierarchyTimeIntegrator"),
            patch_hierarchy,
            level_strategy);

        //  ***** 创建可视化输出器类
        tbox::Pointer<appu::JaVisDataWriter<NDIM> > javis_data_writer;
        if (javis_dump_interval > 0)
        { // 输出时间步间隔，0表示关闭
                javis_data_writer = new appu::JaVisDataWriter<NDIM>("JaVis_Writer", javis_dump_dirname, javis_number_procs_per_file);
                // 绘图量命名为plotval，通过绘图数据片的数据生成.javis文件
                javis_data_writer->registerPlotQuantity("plotval", "SCALAR", patch_strategy->d_plot_id);
        }

        /*****************************************************************************************
                                                                                              4. 计算
                4.1 初始化计算；4.2 时间步推进求解
        *****************************************************************************************/
        //4.1初始化计算
        time_integrator->initializeHierarchy();

        // ***** 输出可视化数据场到可视化输出器
        if (javis_dump_interval > 0)
        {
                javis_data_writer->writePlotData(patch_hierarchy,
                                                 time_integrator->getIntegratorStep(),
                                                 time_integrator->getIntegratorTime());
        }

        // 4.2 时间步推进求解
        double loop_time = time_integrator->getIntegratorTime();  // 时间积分当前时刻
        double loop_time_end = time_integrator->getEndTime();     // 时间积分终止时刻
        int iteration_num = time_integrator->getIntegratorStep(); // 当前时间步序号

        while ((loop_time < loop_time_end) && time_integrator->stepsRemaining())
        {
                //      未达到终止时刻                  所有时间步未积分完毕
                tbox::pout << "\n\n++++++++++++++++++++++++++++++++++++++++++++" << endl;
                tbox::pout << "At begining of timestep # " << iteration_num << endl;
                tbox::pout << "Simulation time is " << loop_time << endl;
                tbox::pout << "++++++++++++++++++++++++++++++++++++++++++++\n\n"
                           << endl;

                // 计算时间步长，将数值解推进一个时间步
                double dt_actual = time_integrator->advanceHierarchy();
                loop_time += dt_actual;
                iteration_num = time_integrator->getIntegratorStep();

                tbox::pout << "\n\n++++++++++++++++++++++++++++++++++++++++++++" << endl;
                tbox::pout << "At end of timestep # " << iteration_num << endl;
                tbox::pout << "Dt = " << dt_actual << ", Simulation time is " << loop_time << endl;
                tbox::pout << "++++++++++++++++++++++++++++++++++++++++++++\n\n"
                           << endl;

                if ((javis_dump_interval > 0) &&
                    (iteration_num % javis_dump_interval) == 0)
                {
                        javis_data_writer->writePlotData(patch_hierarchy, iteration_num,
                                                         loop_time);
                }
        }

        // 灵敏指针指向的内存空间由JASMIN框架管理，对用户定义的类使用标准指针，传到JASMIN框架中不会释放内存，需用户管理释放
        if (level_strategy)
                delete level_strategy;
        if (patch_strategy)
                delete patch_strategy;
        // 输出记时器统计的时间数据
        tbox::TimerManager::getManager()->print(tbox::plog);
        /*****************************************************************************************
                                                                                              5. 终止
        *****************************************************************************************/
        // 释放JAUMIN和MPI内部资源
        tbox::JAUMINManager::shutdown();
        tbox::MPI::finalize();
        return 0;
}
