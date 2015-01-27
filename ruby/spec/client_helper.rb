# This is the spec helper for Smith::Client end to end integration tests

require 'rspec/em'

require 'common_helper'
require 'smith/client'

# Require the steps
Dir[File.expand_path('../support/client_steps/*.rb', __FILE__)].each { |f| require(f) }

# Enable/disable printing client log messages to stdout
$client_log_enable = false
# Enable/disable printing VCR log messages to stdout
$vcr_log_enable = false
# Enable/disable printing Faye log messages to stdout
$faye_log_enable = false

# Allow overriding of per test timeout
if timeout = ENV['CLIENT_TEST_TIMEOUT']
  client_test_timeout = timeout.to_f
else
  client_test_timeout = 4.0
end

# Allow overriding of expectation timeouts
# This is used for timing out when expected HTTP requests and command pipe writes are
# not observed in dummy_server_helper.rb and print_engine_helper_async.rb
if timeout = ENV['CLIENT_EXPECTATION_TIMEOUT']
  $client_expectation_timeout = timeout.to_f
else
  $client_expectation_timeout = 2.0
end

RSpec.configure do |config|
  config.include(DummyServerHelper, :client)
  config.include(ClientHelper, :client)
  config.include(Smith::Client::URLHelper, :client)
  config.include(VCRSteps, :vcr)

  config.before(:suite) do
    # Start the dummy server in child process once before all tests
    $dummy_server = DummyServer.new
    $dummy_server.start
  end

  config.after(:suite) do
    $dummy_server.stop
  end

  config.before(:each, :client) do
    # Start a watchdog timer to timeout any tests that don't finish in a reasonable amount of time
    EM.next_tick do
      @watchdog_timer = EM.add_timer(client_test_timeout) { raise 'timeout waiting for test to run' }
    end
  end

  config.after(:each, :client) do
    # Cancel the watchdog timer when a test completes
    @watchdog_timer.cancel if @watchdog_timer
  end

  config.before(:all, :vcr) do
    VCR.configure do |c|
      c.cassette_library_dir = 'cassettes'
      c.hook_into :webmock
      c.ignore_hosts 'localhost', '127.0.0.1', 'bad.url'
      c.debug_logger = STDOUT if $vcr_log_enable
    end
  end

  config.before(:each, :vcr) do |example|
    insert_vcr_cassette(example.metadata[:full_description])
  end

  config.after(:each, :vcr) do
    eject_vcr_cassette
  end

end
