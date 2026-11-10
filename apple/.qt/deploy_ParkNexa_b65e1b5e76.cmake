include("C:/Users/kumar/OneDrive/Desktop/mhk/ParkNexa/build/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/ParkNexa-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "C:/Users/kumar/OneDrive/Desktop/mhk/ParkNexa/build/ParkNexa.exe"
    GENERATE_QT_CONF
)
