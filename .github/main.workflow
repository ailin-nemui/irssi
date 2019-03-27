workflow "Check Irssi" {
  on = "push"
  resolves = ["script"]
}

action "install" {
  uses = "ailin-nemui/actions-irssi/check-irssi@master"
  args = ".github/actions.yml before_install install"
}

action "script" {
  uses = "ailin-nemui/actions-irssi/check-irssi@master"
  needs = ["install"]
  args = ".github/actions.yml before_script script after_script"
}
