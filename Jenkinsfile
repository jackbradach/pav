#!/usr/bin/env groovy

node() {
    def img
    stage("Checkout source code") {
        checkout scm
        //git credentialsId: 'jenkinsci-slave', url: ' ssh://git@bitbucket.org/jackbradach/pav.git'
    }

    stage("Prepare Container") {
        img = docker.build("jbradach/build_pav", ".")
    }

    img.inside {
        stage("Building Binary and Package") {
            sh """
            mkdir -p release
            cd release
            cmake -DCMAKE_BUILD_TYPE=Release ..
            make
            make package
            cd ..
            """
            archive includes:'release/bin/pav,release/*.deb'
        }

        stage("Generating Coverage Report") {
            sh """
            mkdir -p coverage
            cd coverage
            cmake -DCMAKE_BUILD_TYPE=Coverage ..
            make
            make coverage
            cd ..
            """
            publishHTML([
                allowMissing: true,
                alwaysLinkToLastBuild: true,
                keepAll: true,
                reportDir: 'coverage/cov/',
                reportFiles: 'index.html',
                reportName: 'Coverage (lcov)'
            ])
        }

    }


    stage("Cleanup") {
        deleteDir()
    }

}
