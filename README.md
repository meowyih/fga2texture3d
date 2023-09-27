# FGA to Godot Texture3D Convertor

FGA (Fluid Grid Ascii) is a format vector field. In Vectoraygen or Unreal Engine, it can direct be imported into project / material. But even Godot has GPUParticlesAttractorVectorField3D Node, it cannot read FGA directly. This simple C++ file shows how to read the FGA and convert to Godot readable image, which can directly import into Godot project with correct Texture3D slice setting.


## Build Requirement

This is just a main.cpp code with STL library. But since it uses *auto* keyword, C++17 is needed.

## Usage

- Input File Name

Change the *fga_filename*

- Output File Name

The output file name is in the following format: **t3d_w{$H_SLICE_SIZE}_h{$V_SLICE_SIZE}.bmp** For example, *t3d_w10_h1.bmp* is a Texture3D BMP file that slices into 1 column and 10 row flipbook style image. Use the **{$H_SLICE_SIZE}** and **{$V_SLICE_SIZE}** to import the Texture3D correctly in Godot.

- Slice Configuration

**MAX_SLICE_SIZE** constant in code indicate the max **{$H_SLICE_SIZE}** size during image generation. Beware that Godot Texture3D importer has V:256 H:256 limitation for slice. But usually field vector does not has 256x256=65535 depth. It should not be a problem.

## Possible Issue ##

I only test the FGA file that generate by **Vectoraygen**. The code wasn't fully tested in all possible FGA files.
