node("docker") {
    registry_url = "https://index.docker.io/v1/"
    registry_creds_id = "dockerhub-jbradach"
    build_tag = "testing"

    docker.withRegistry('${registry_url}', '${registry_creds_id}') {

        git url: "git@bitbucket.org:jackbradach/pav.git", credentialsId: 'jackbradach'

        sh "git rev-parse HEAD > .git/commit-id"
        def commit_id = readFile('.git/commit-id').trim()
        println commit_id

        stage "Building"
        def app = docker.build "pav"

    }
}

node {


}
