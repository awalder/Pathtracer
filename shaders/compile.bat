glslangValidator.exe -V simple.vert -o spirv/vertshader.spv
glslangValidator.exe -V simple.frag -o spirv/fragshader.spv
glslangValidator.exe -V AO.rmiss -o spirv/AO.rmiss.spv
glslangValidator.exe -V AO_shadow.rmiss -o spirv/AO_shadow.rmiss.spv
glslangValidator.exe -V AO.rgen -o spirv/AO.rgen.spv
glslangValidator.exe -V AO.rchit -o spirv/AO.rchit.spv
glslangValidator.exe -V pathRT.rchit -o spirv/pathRT.rchit.spv
glslangValidator.exe -V pathRT.rgen -o spirv/pathRT.rgen.spv
glslangValidator.exe -V pathRT.rmiss -o spirv/pathRT.rmiss.spv
glslangValidator.exe -V pathRTBounce.rchit -o spirv/pathRTBounce.rchit.spv
glslangValidator.exe -V pathRTBounce.rmiss -o spirv/pathRTBounce.rmiss.spv

pause