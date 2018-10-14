if exist('accErrors') == 0
    accErrors = [];
end
accErrors(:, size(accErrors, 2) + 1) = csvread('D:/data/out/error.csv');