del /s /q .gradle
rmdir /s /q .gradle
del /s /q .idea
rmdir /s /q .idea

del /s /q app\.externalNativeBuild
rmdir /s /q app\.externalNativeBuild
del /s /q app\build
rmdir /s /q app\build

del /s /q vsense-libs\.externalNativeBuild
rmdir /s /q vsense-libs\.externalNativeBuild
del /s /q vsense-libs\build
rmdir /s /q vsense-libs\build
del /s /q vsense-libs\lib
rmdir /s /q vsense-libs\lib

del app\*.iml
del vsense-libs\*.iml
del *.iml
del local.properties