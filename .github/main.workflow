workflow "Check Irssi" {
  on = "push"
  resolves = [
    "script",
    "unit_tests",
  ]
}

action "install" {
  uses = "ailin-nemui/actions-irssi/check-irssi@master"
  args = ".github/actions.yml before_install install"
}

action "script" {
  uses = "ailin-nemui/actions-irssi/check-irssi@master"
  needs = ["install"]
  args = ".github/actions.yml before_script script after_script"
  env = {
    TERM = "xterm"
  }
}

action "unit_tests" {
  uses = "ailin-nemui/actions-irssi/check-irssi@master"
  needs = ["install"]
  args = ".github/actions.yml unit_tests after_unit_tests"
}
