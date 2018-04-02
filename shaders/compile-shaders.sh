#!/bin/bash

GLSL_VALIDATOR_PATH="/c/VulkanSDK/1.1.70.0/Bin/glslangValidator.exe"

echo "Using GLSL Validator at $GLSL_VALIDATOR_PATH"
echo ""

$GLSL_VALIDATOR_PATH -V ./*.vert
$GLSL_VALIDATOR_PATH -V ./*.frag