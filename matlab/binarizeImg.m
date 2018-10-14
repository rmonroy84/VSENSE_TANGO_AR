function binarizeImg(filename, outFile)

img = imread(filename);

nbrRows = uint32(size(img, 1));
nbrCols = uint32(size(img, 2));
nbrChans = min(3, uint32(size(img, 3)));

fid = fopen(outFile, 'wb');

fwrite(fid, nbrRows, 'uint32');
fwrite(fid, nbrCols, 'uint32');
fwrite(fid, nbrChans, 'uint32');

for row = 1:nbrRows
   for col = 1:nbrCols
      for chan = 1:nbrChans
          fwrite(fid, img(row, col, chan), 'uint8');
      end
   end    
end

fclose(fid);

end