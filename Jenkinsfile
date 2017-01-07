node() {
    def img
    stage("Checkout source code") {
        checkout scm
    }

    stage("Prepare Container") {
        img = docker.build('jbradach/build_pav')
    }

    img.inside {
        stage("Container Preparation")
        sh """
        mkdir build
        cd build
        cmake -DCMAKE_BUILD_TYPE=Coverage ..
        make coverage
        """

    }

    stage("Cleanup") {
        deleteDir()
    }

}
