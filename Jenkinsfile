#!/usr/bin/env groovy

node() {
    def img
    stage("Checkout source code") {
        checkout scm
        //git credentialsId: 'jenkinsci-slave', url: ' ssh://git@bitbucket.org/jackbradach/pav.git'
    }

    stage("Prepare Containers") {
        img = docker.build("jbradach/build_pav", ".")
    }

    img.inside {
        stage("Building Release") {
            sh """
            mkdir -p Release
            cd Release
            cmake -DCMAKE_BUILD_TYPE=Release ..
            make
            make package
            cd ..
            """
            archive includes:'**/Release/bin/pav,**/Release/*.deb'
        }

        stage("Generating Test Report") {
            sh """
            mkdir -p Debug
            cd Debug
            cmake -DCMAKE_BUILD_TYPE=Debug ..
            make
            make check
            cd ..
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

        stage("Generating Coverage Report") {
            sh """
            mkdir -p Coverage
            cd Coverage
            cmake -DCMAKE_BUILD_TYPE=Coverage ..
            make
            make coverage
            cd ..
            """
            publishHTML([
                alwaysLinkToLastBuild: true,
                keepAll: true,
                reportDir: 'Coverage/cov',
                reportFiles: 'index.html',
                reportName: 'Coverage (lcov)'
            ])
        }
    }

    stage("Cleanup") {
        deleteDir()
    }

}
