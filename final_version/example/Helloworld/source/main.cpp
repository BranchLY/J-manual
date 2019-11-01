#include <string>
#include "jaumin/MPI.h"
#include "jaumin/JAUMINManager.h"

int main(int argc, char* argv[]){
	//初始化MPI和JAUMIN环境。
	JAUMIN::tbox::MPI::init(&argc, &argv);
	JAUMIN::tbox::JAUMINManager::startup();

	//输出
	std::cout << "Hello,world!" << std::endl;

	//释放JAUMIN和MPI内部资源
	JAUMIN::tbox::JAUMINManager::shutdown();
	JAUMIN::tbox::MPI::finalize();

	return(0);
}