#!/usr/bin/env groovy

node() {
    def img
    stage("Checkout source code") {
//        checkout scm
    }

    stage("Prepare Container") {
        img = docker.build("build_pav-$BUILD_NUMBER")
    }

    img.inside {
        stage("Container Preparation")
        sh """
        mkdir -f build
        cd build
        cmake -DCMAKE_BUILD_TYPE=Coverage ..
        make coverage
        """
    }

    stage("Cleanup") {
        deleteDir()
    }

}
