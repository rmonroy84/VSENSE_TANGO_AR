function hsv = rgb2hsv(rgb)
    hsv = [0 0 0];
	sRGB = linRGB2sRGB(rgb);
	
	minVal = min(sRGB(1), min(sRGB(2), sRGB(3)));
	maxVal = max(sRGB(1), max(sRGB(2), sRGB(3)));
	chroma = maxVal - minVal;	

	hsv(3) = maxVal;	

	if (chroma ~= 0) 
		hsv(2) = chroma / maxVal;

		if (sRGB(1) == maxVal)
			hsv(1) = (sRGB(2) - sRGB(3)) / chroma;
			
			if (sRGB(1) < sRGB(3))
				hsv(1) = hsv(1) + 6;	
            end
        elseif (sRGB(2) == maxVal)
			hsv(1) = 2 + ((sRGB(3) - sRGB(1)) / chroma);
        elseif (sRGB(3) == maxVal)
			hsv(1) = 4 + ((sRGB(1) - sRGB(2)) / chroma);
        end
        
		hsv(1) = hsv(1)/6;

		if (hsv(1) < 0)
			hsv(1) = 1.0 - hsv(1);
        end
    end    
end