workflow "Check Irssi" {
  on = "push"
  resolves = [
    "script",
    "unit_tests",
    "ailin-nemui/actions-irssi/windows@master",
  ]
}

action "install" {
  uses = "ailin-nemui/actions-irssi/check-irssi@master"
  args = "before_install install"
}

action "script" {
  uses = "ailin-nemui/actions-irssi/check-irssi@master"
  needs = ["install"]
  args = "before_script script after_script"
  env = {
    TERM = "xterm"
  }
}

action "unit_tests" {
  uses = "ailin-nemui/actions-irssi/check-irssi@master"
  needs = ["install"]
  args = "unit_tests after_unit_tests"
}

action "ailin-nemui/actions-irssi/windows@master" {
  uses = "ailin-nemui/actions-irssi/windows@master"
}
