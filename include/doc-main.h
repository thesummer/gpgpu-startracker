#ifndef MAINPAGE_H
#define MAINPAGE_H

/*!
  @mainpage OpenGL ES2.0 gpgpu-labeling for startracking

    Star trackers are essential devices in spacecaft for attitude determination
    without any previous knowledge of the spacecraft's attitude. In contrast to
    other optical sensors like earth or sun sensor they also they also allow
    for a high accuracy in the determined attitude.

    In a top-down view attitude determination in a star tracker can be split up
    in 4 different stages:
        - Taking the picture
        - Extracting bright spots from the picture as supposed stars
        - Indentify the stars by comparison with stars from a known catalog
        - Compute the attitude using the image vectors of the identified stars


    In a previous attempt to create a star tracker from cheap of the shelf (COTS)
    parts, mainly a BeagleBone embedded linux board connected to a camera
    mainly used for CCTV, several things were noted.

    First, the camera had a comparably low sensitivity, hence in field tests
    only very bright stars of magnitude 3 and smaller could be recognized from
    background noise. This has the beneficial side effect that only a very small
    fraction of stars have to be kept in a reference star catalog and much less
    possible patterns have to be searched through in the algorithm for
    identification of the individual stars extracted from the night sky image.
    Additionally the larger opening angle and hence the larger field-of-view
    still enabled us to have on average around 8 stars per image available for
    the identification process.

    However, the used camera and its lens system showed a large amount of
    distortion. It could be compensated in software to a large extend by
    taking a large amount of images of checkerboards and determine the
    distortion and displacement coefficients of the optical system.

    The main bottleneck turned out to be the extraction of the star spots from
    the night sky pictures. Due to comparably large camera resolution of 5 MP
    each image has a size of around 16 MB. During the star spot extraction
    besides some simple image operations a labeling process is necessary in
    order to identify all pixel which belong to one star spot. Then, the
    weighted centroid of this spot is computed, the distortion correction is
    applied and finallly the results are passed to the identification stage.
    The labeling operation however requires to pass through each pixel at least
    twice and includes many random write operations. This results in a time for
    the spot extraction of TODO ms per image whereas the whole identification
    stage only needs TODO ms.

    @image html cpu-gpu-step-0.svg "Figure 1 Previous setup on the BeagleBone"

    The target platform is the raspberryPi micro computer.
    In comparison with the previously used BeagleBone the CPU of the raspberryPi
    is significantly less powerful. However, the SoC features a GPU as a co-processor.
    The GPU can be programmed via the OpenGL ES2.0 API.
    The goal of this project is therefore to find ways to speed-up the spot
    extraction process using the GPU. It should process the raw image, identify the
    spots and compute the necessary statistics for each spot in the graphic memory.
    Only the results are then copied back to the RAM of the CPU and are used for the
    star identification.


    At the moment images are read from file and copied to the video memory before
    processing starts.

    @image html cpu-gpu-step-1.svg "Figure 2 current memory layout on the RaspberryPi"

    A later milestone would be to use the raspberryPi camera
    to take pictures of the night sky (directly to video memory) and process the
    images on the fly.

    @image html cpu-gpu-step-2.svg "Figure 3 Intended memory layout on the RaspberryPi"


    @ref labeling
    @ref reduction
    @ref stats
*/



#endif
