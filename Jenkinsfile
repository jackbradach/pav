#!/usr/bin/env groovy

node() {
    def img
    stage("Checkout source code") {
        checkout scm
    }

    stage("Prepare Container") {
        img = docker.build("jbradach/build_pav", ".")
    }

    img.inside {
        stage("Container Preparation")
        sh """
        mkdir -p build
        cd build
        cmake -DCMAKE_BUILD_TYPE=Coverage ..
        make
        make coverage
        """
    }

    stage("Generating Coverage Report") {
        publishHTML([allowMissing: true,
                     alwaysLinkToLastBuild: true,
                     keepAll: true,
                     reportDir: 'cov/',
                     reportFiles: 'index.html',
                     reportName: 'Coverage (lcov)'])
    }

    stage("Cleanup") {
        deleteDir()
    }

}
