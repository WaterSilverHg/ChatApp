// oatpptest.cpp: 定义应用程序的入口点。
//

#include "global.h"
#include"server/server.cpp"

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    oatpp::base::Environment::init();

    try {
        run();

    }
    catch (const std::runtime_error& e) {
        std::cerr << "Runtime error caught: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "An unknown exception occurred." << std::endl;
        return EXIT_FAILURE;
    }

    oatpp::base::Environment::destroy();
	return 0;
}
