#!/bin/bash

q00_t01="(def string x y (* 120 (+ x y))"
q00_r01=$(./question_01 "$q00_t01")

echo "string $q00_t01 balance score : $q00_r01"


q00_t02="(def string x y (* 120 (+ x y)))"
q00_r02=$(./question_01 "$q00_t02")

echo "string $q00_t02 balance score : $q00_r02"