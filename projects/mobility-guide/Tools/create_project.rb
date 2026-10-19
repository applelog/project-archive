require 'xcodeproj'

project = Xcodeproj::Project.new('MobilityGuide.xcodeproj')
target = project.new_target(:application, 'MobilityGuide', :ios, '17.0')

{ 'App' => %w[MobilityGuideApp.swift LocationService.swift ContentView.swift],
  'Core' => %w[RouteModel.swift Router.swift StationModel.swift] }.each do |folder, names|
  group = project.main_group.new_group(folder, folder)
  names.each do |name|
    target.source_build_phase.add_file_reference(group.new_file(name))
  end
end

data_group = project.main_group.new_group('Data', 'Data')
%w[network.json station_steps.json].each do |name|
  target.resources_build_phase.add_file_reference(data_group.new_file(name))
end

target.build_configurations.each do |configuration|
  configuration.build_settings['PRODUCT_BUNDLE_IDENTIFIER'] = 'com.example.MobilityGuide'
  configuration.build_settings['INFOPLIST_KEY_CFBundleDisplayName'] = '이동권 길찾기'
  configuration.build_settings['SWIFT_VERSION'] = '5.0'
  configuration.build_settings['IPHONEOS_DEPLOYMENT_TARGET'] = '17.0'
  configuration.build_settings['TARGETED_DEVICE_FAMILY'] = '1'
  configuration.build_settings['GENERATE_INFOPLIST_FILE'] = 'YES'
  configuration.build_settings['INFOPLIST_KEY_NSLocationWhenInUseUsageDescription'] =
    '현재 위치에서 접근 가능한 보행 경로를 계산하기 위해 사용 중 위치가 필요합니다.'
  configuration.build_settings['INFOPLIST_KEY_UIApplicationSceneManifest_Generation'] = 'YES'
  configuration.build_settings['INFOPLIST_KEY_UILaunchScreen_Generation'] = 'YES'
  configuration.build_settings['SWIFT_STRICT_CONCURRENCY'] = 'targeted'
end

project.save
