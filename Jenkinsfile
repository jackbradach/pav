node {
    stage 'checkout' {
        checkout scm
    }

    stage 'build'
    def app = docker.build "jbradach/pav:${env.BUILD_NUMBER}"


}
