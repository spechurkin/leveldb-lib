Gem::Specification.new do |spec|
  spec.name = 'multileveldb'
  spec.version = '1.0.0'
  spec.summary = 'Binary-safe LevelDB access through the MultiLevelDB C ABI'
  spec.authors = ['MultiLevelDB contributors']
  spec.license = '0BSD'
  spec.required_ruby_version = '>= 3.1'
  spec.files = Dir['lib/**/*.rb'] + ['README.md', 'LICENSE']
  spec.add_dependency 'fiddle', '>= 1.1'
  spec.metadata['rubygems_mfa_required'] = 'true'
  repository = ENV.fetch('GITHUB_REPOSITORY', '')
  repository = 'spechurkin/leveldb-lib' if repository.empty?
  spec.homepage = "https://github.com/#{repository}"
  spec.metadata['source_code_uri'] = spec.homepage
  spec.metadata['bug_tracker_uri'] = "#{spec.homepage}/issues"
  spec.metadata['github_repo'] = "ssh://github.com/#{repository}"
end
