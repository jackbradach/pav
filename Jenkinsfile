node {
    stage 'Checkout' {
        checkout scm
    }

    stage 'Build'
    def app = docker.build "jbradach/pav:${env.BUILD_NUMBER}"

    
}
