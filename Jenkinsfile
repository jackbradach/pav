#!/usr/bin/env groovy

node() {
    def img
    stage("Checkout source code") {
        checkout scm
        //git credentialsId: 'jenkinsci-slave', url: ' ssh://git@bitbucket.org/jackbradach/pav.git'
    }

    stage("Prepare Containers") {
        img_release = docker.build("jbradach/build_pav", ".")
        img_debug = docker.build("jbradach/build_pav", ".")
        img_coverage = docker.build("jbradach/build_pav", ".")
    }

    img_release.inside {
        stage("Building Release") {
            sh """
            mkdir -p Release
            cd Release
            cmake -DCMAKE_BUILD_TYPE=Release ..
            make
            make package
            """
            archive includes:'**/Release/bin/pav,**/Release/*.deb'
        }
    }

    img_debug.inside {
        stage("Generating Test Report") {
            sh """
            mkdir -p Debug
            cd Debug
            cmake -DCMAKE_BUILD_TYPE=Debug ..
            make
            make check
            """
            step([$class: 'XUnitBuilder',
                testTimeMargin: '3000',
                thresholdMode: 1,
                thresholds: [[$class: 'FailedThreshold', unstableThreshold: '1']],
                tools: [[$class: 'GoogleTestType',
                            deleteOutputFiles: true,
                            failIfNotNew: true,
                            pattern: '**/Debug/reports/*.xml',
                            stopProcessingIfError: true]]])
        }
    }

    img_coverage.inside {
        stage("Generating Coverage Report") {
            sh """
            mkdir -p Coverage
            cd Coverage
            cmake -DCMAKE_BUILD_TYPE=Coverage ..
            make
            make coverage
            """
            publishHTML([
                allowMissing: true,
                alwaysLinkToLastBuild: true,
                keepAll: true,
                reportDir: '**/Coverage/cov/',
                reportFiles: 'index.html',
                reportName: 'Coverage (lcov)'
            ])
        }
    }

    stage("Cleanup") {
        deleteDir()
    }

}
