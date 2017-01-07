node() {
    stage("Checkout source code") {
        checkout scm
    }

    stage("Prepare Container") {
        def env = docker.build('jbradach/build_pav')
    }

    env.inside {
        stage("Container Preparation")
        sh """"
        ls -l
        """

    }


    stage("Cleanup") {
        deleteDir()
    }

}
