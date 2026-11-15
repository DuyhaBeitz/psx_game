#pragma usage opaque shadow

uniform vec2 u_snap_resolution;
noperspective varying vec2 v_uv;

void vertex() {
    vec4 world_pos = MATRIX_MODEL * vec4(POSITION, 1.0);
    
    vec4 clip_pos = MATRIX_VIEW_PROJECTION * world_pos;
    
    if (clip_pos.w != 0.0) {
        vec2 ndc = clip_pos.xy / clip_pos.w;
        
        // Snap coordinates to screen-space resolution grid
        vec2 snapped_ndc = floor(ndc * u_snap_resolution) / u_snap_resolution;
        
        // Convert snapped clip-space offset back to local POSITION
        vec4 snapped_clip = vec4(snapped_ndc * clip_pos.w, clip_pos.zw);
        vec4 snapped_world = inverse(MATRIX_VIEW_PROJECTION) * snapped_clip;
        
        // Update local mesh position (R3D auto-composes it with instances)
        POSITION = (inverse(MATRIX_MODEL) * snapped_world).xyz;
    }

    v_uv = TEXCOORD;
}

void fragment() {
    FetchMaterial(v_uv);
}