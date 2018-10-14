clear all
clc

samples = csvread('samples.csv');
matA = csvread('matA.csv');
matB = csvread('matB.csv');
matCorr = csvread('matCorr.csv');

sRef = samples(:, 1:3);
sCur = samples(:, 4:6);