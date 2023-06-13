//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

struct VSInput
{
    float3 position : POSITION;
    float2 uv : UV;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : UV;
};

PSInput VSMain(VSInput input)
{
    PSInput result;

    result.position = float4(input.position.xyz, 1.0f);
    result.uv = input.uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.uv, 0.0f, 1.0f);
}
