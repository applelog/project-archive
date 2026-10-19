package main

import (
	"strings"
	"testing"
)

func TestInitialModel(t *testing.T) {
	m := initialModel()
	view := m.View()
	expected := "나의 불편함"
	if !strings.Contains(view, expected) {
		t.Errorf("Expected view to contain %q, got %q", expected, view)
	}
}

